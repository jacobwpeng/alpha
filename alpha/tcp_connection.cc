/*
 * =============================================================================
 *
 *       Filename:  tcp_connection.cc
 *        Created:  04/05/15 12:05:06
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include "tcp_connection.h"

#include <sys/uio.h>
#include <cassert>
#include "compiler.h"
#include "logger.h"
#include "channel.h"
#include "event_loop.h"
#include "socket_ops.h"

namespace alpha {
    TcpConnection::TcpConnection(EventLoop* loop, int fd, TcpConnection::State state)
        :loop_(loop), fd_(fd), state_(state) {
            assert (loop_);
            assert (fd_);
            channel_.reset (new Channel(loop, fd));
            assert ( state_ != State::kDisconnected);
            if (state_ == State::kConnecting) {
                InitConnecting();
            } else {
                assert (state_ == State::kConnected);
                InitConnected();
            }
            channel_->set_error_callback(std::bind(&TcpConnection::HandleError, this));
        }

    TcpConnection::~TcpConnection() {
        DLOG_INFO << "TcpConnection destroyed, fd = " << fd_;
        channel_->Remove();
        ::close(fd_);
    }

    void TcpConnection::Write(const alpha::Slice& data) {
        if (unlikely(write_buffer_.Append(data) == false)) {
            LOG_WARNING << "Write failed, data.size() = " << data.size()
                << ", local_addr_ = " << *local_addr_
                << ", peer_addr_ = " << *peer_addr_;
        }
        channel_->EnableWriting();
    }
    
    void TcpConnection::Close() {
        assert (state_ != State::kDisconnected);
        if (state_ == State::kConnected) {
            channel_->DisableReading();
            SocketOps::DisableReading(fd_);
        } else {
            assert (state_ == State::kConnecting);
            channel_->DisableWriting();
        }

        state_ = State::kDisconnected;
        DLOG_INFO << "TcpConnection closed, fd = " << fd_;
        if (close_callback_) {
            //给个机会把connection里面缓存的写数据写出去
            loop_->QueueInLoop(std::bind(close_callback_, fd_));
        }
    }

    void TcpConnection::CloseByPeer() {
        assert (state_ == State::kConnected);
        channel_->DisableReading();
        channel_->DisableWriting();
        state_ = State::kDisconnected;
        DLOG_INFO << "TcpConnection closed by peer, fd = " << fd_;
        if (close_callback_) {
            //给个机会把connection里面缓存的写数据写出去
            loop_->QueueInLoop(std::bind(close_callback_, fd_));
        }
    }

    void TcpConnection::ReadFromPeer() {
        static const size_t kLocalBufferSize = 1 << 16; // 使用栈内存和readv尽快读取数据
        char local_buffer[kLocalBufferSize];
        const int iovcnt = 2;
        iovec iov[iovcnt];

        size_t contiguous_space_in_buffer = read_buffer_.GetContiguousSpace();
        iov[0].iov_base = read_buffer_.WriteBegin();
        iov[0].iov_len = contiguous_space_in_buffer;
        iov[1].iov_base = local_buffer;
        iov[1].iov_len = sizeof(local_buffer);

        ssize_t nbytes = ::readv(fd_, iov, iovcnt);
        DLOG_INFO << "nbytes = " << nbytes;

        if (nbytes == 0) {
            LOG_INFO << "Peer closed connection, local_addr_ = " << *local_addr_
                << ", peer_addr_ = " << *peer_addr_;
            CloseByPeer();
        } else if (nbytes == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            PLOG_WARNING << "readv failed";
        } else {
            size_t bytes = static_cast<size_t>(nbytes);
            if (bytes <= contiguous_space_in_buffer) {
                read_buffer_.AddBytes(bytes);
            } else {
                read_buffer_.AddBytes(contiguous_space_in_buffer);
                size_t local_buffer_bytes = bytes - contiguous_space_in_buffer;
                alpha::Slice data(local_buffer, local_buffer_bytes);
                bool ok = read_buffer_.Append (data);
                if (unlikely(!ok)) {
                    LOG_WARNING << "Append read_buffer_ failed, bytes = " << bytes
                        << ", contiguous_space_in_buffer = " << contiguous_space_in_buffer
                        << ", local_buffer_bytes = " << local_buffer_bytes
                        << ", local_addr_ = " << *local_addr_ 
                        << ", peer_addr_ = " << *peer_addr_;
                        
                }
            }
            DLOG_INFO << "Read " << bytes << " bytes from " << *peer_addr_;
            if (read_callback_) {
                read_callback_(shared_from_this(), &read_buffer_);
            }
        }
    }

    void TcpConnection::WriteToPeer() {
        alpha::Slice data = write_buffer_.Read();
        size_t bytes = data.size();

        while (bytes != 0) {
            //TODO: 使用writev调整Buffer写入策略，同时快速递送消息
            ssize_t nbytes = ::write(fd_, data.data(), bytes);
            if (nbytes == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    //写满了
                } else if (state_ == State::kDisconnected) {
                    // 连接已经关闭，而发送又失败了，就放弃吧
                    PLOG_WARNING << "write to closed connection failed, bytes = " << bytes;
                } else {
                    PLOG_WARNING << "write failed, bytes = " << bytes 
                        << ", peer_addr_ = " << *peer_addr_;
                }
                break;
            }
            write_buffer_.ConsumeBytes(nbytes);
            bytes -= nbytes;
            DLOG_INFO << "Write " << nbytes << " bytes to " << *peer_addr_;
        }

        if (bytes == 0) {
            channel_->DisableWriting();
        }
    }

    void TcpConnection::ConnectedToPeer() {
        assert (state_ == State::kConnecting);
        int err = SocketOps::GetAndClearError(fd_);
        if (err != 0) {
            char buf[128];
            LOG_WARNING << "Error: " << ::strerror_r(err, buf, sizeof(buf))
                << ", peer_addr_ = " << *peer_addr_;
        } else {
            state_ = State::kConnected;
            GetAddressInfo();
            channel_->EnableReading();
            //TcpConnection生命周期大于channel_
            channel_->set_read_callback(std::bind(&TcpConnection::ReadFromPeer, this));
            channel_->DisableWriting();
            channel_->set_write_callback(std::bind(&TcpConnection::WriteToPeer, this));

            DLOG_INFO << "Connected to " << *peer_addr_;
            if (connected_callback_) {
                connected_callback_(shared_from_this());
            }
        }
    }

    void TcpConnection::HandleError() {
        int err = SocketOps::GetAndClearError(fd_);
        char buf[128];
        if (err == ECONNRESET) {
            // 不把这个当做是错误
            LOG_INFO << "Connection reset by " << *peer_addr_;
        } else {
            LOG_WARNING_IF(err) << "Error: " << ::strerror_r(err, buf, sizeof(buf))
                << ", peer_addr_ = " << *peer_addr_;
        }
    }

    void TcpConnection::GetAddressInfo() {
        assert (state_ == State::kConnected);
        assert(!local_addr_);

        local_addr_.reset(new NetAddress(NetAddress::GetLocalAddr(fd_)));
        auto peer_addr = NetAddress::GetPeerAddr(fd_);
        if (peer_addr_) {
            assert (*peer_addr_ == peer_addr);
        } else {
            SetPeerAddr(NetAddress::GetPeerAddr(fd_));
        }
    }

    void TcpConnection::SetPeerAddr(const alpha::NetAddress& addr) {
        assert (!peer_addr_);
        peer_addr_.reset (new NetAddress(addr));
    }

    void TcpConnection::InitConnecting() {
        channel_->set_write_callback(std::bind(&TcpConnection::ConnectedToPeer, this));
        channel_->EnableWriting();
    }

    void TcpConnection::InitConnected() {
        channel_->set_read_callback(std::bind(&TcpConnection::ReadFromPeer, this));
        channel_->set_write_callback(std::bind(&TcpConnection::WriteToPeer, this));
        channel_->EnableReading();
        GetAddressInfo();
    }
}
