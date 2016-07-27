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
#include <alpha/compiler.h>
#include <alpha/logger.h>
#include <alpha/channel.h>
#include <alpha/event_loop.h>
#include <alpha/socket_ops.h>

namespace alpha {
TcpConnection::TcpConnection(EventLoop* loop, int fd,
                             TcpConnection::State state)
    : loop_(loop), fd_(fd), state_(state) {
  DCHECK(loop_);
  DCHECK(fd_);
  channel_.reset(new Channel(loop, fd));
  DCHECK(state_ == State::kConnected);
  Init();
}

TcpConnection::~TcpConnection() {
  DLOG_INFO << "TcpConnection destroyed, fd = " << fd_;
  channel_->Remove();
  ::close(fd_);
}

bool TcpConnection::Write(const void* data, size_t size) {
  if (unlikely(write_buffer_.Append(data, size) == false)) {
    LOG_WARNING << "Write failed, size = " << size
                << ", local_addr_ = " << *local_addr_
                << ", peer_addr_ = " << *peer_addr_;
    return false;
  }
  channel_->EnableWriting();
  return true;
}

bool TcpConnection::Write(const alpha::Slice& data) {
  return Write(data.data(), data.size());
}

void TcpConnection::Close() {
  DCHECK(state_ != State::kDisconnected);
  if (state_ == State::kConnected) {
    channel_->DisableReading();
    SocketOps::DisableReading(fd_);
  }

  state_ = State::kDisconnected;
  DLOG_INFO << "TcpConnection closed, fd = " << fd_;
  if (close_callback_) {
    // 给个机会把connection里面缓存的写数据写出去
    loop_->QueueInLoop(std::bind(close_callback_, fd_));
  }
}

void TcpConnection::CloseByPeer() {
  DCHECK(state_ == State::kConnected);
  channel_->DisableReading();
  channel_->DisableWriting();
  state_ = State::kDisconnected;
  DLOG_INFO << "TcpConnection closed by peer, fd = " << fd_;
  if (close_callback_) {
    // 正在Channel::HandleEvent, 所以不能直接调用
    loop_->QueueInLoop(std::bind(close_callback_, fd_));
  }
}

void TcpConnection::ReadFromPeer() {
  static const size_t kLocalBufferSize =
      1 << 16;  // 使用栈内存和readv尽快读取数据
  char local_buffer[kLocalBufferSize];
  const int iovcnt = 2;
  iovec iov[iovcnt];

  size_t contiguous_space_in_buffer = read_buffer_.GetContiguousSpace();
  auto space_before_full = read_buffer_.SpaceBeforeFull();
  DCHECK(space_before_full >= contiguous_space_in_buffer);
  iov[0].iov_base = read_buffer_.WriteBegin();
  iov[0].iov_len = contiguous_space_in_buffer;
  iov[1].iov_base = local_buffer;
  iov[1].iov_len = std::min(sizeof(local_buffer),
                            space_before_full - contiguous_space_in_buffer);

  ssize_t nbytes = ::readv(fd_, iov, iovcnt);
  DLOG_INFO << "readv return " << nbytes;

  if (nbytes == 0) {
    LOG_INFO << "Peer closed connection, local_addr_ = " << *local_addr_
             << ", peer_addr_ = " << *peer_addr_;
    CloseByPeer();
  } else if (nbytes == -1) {
    PLOG_WARNING << "readv failed";
  } else {
    size_t bytes = static_cast<size_t>(nbytes);
    if (bytes <= contiguous_space_in_buffer) {
      read_buffer_.AddBytes(bytes);
    } else {
      read_buffer_.AddBytes(contiguous_space_in_buffer);
      size_t local_buffer_bytes = bytes - contiguous_space_in_buffer;
      bool ok = read_buffer_.Append(local_buffer, local_buffer_bytes);
      DCHECK(ok);
      (void)ok;
    }
    DLOG_INFO << "Read " << bytes << " bytes from " << *peer_addr_;
    if (read_callback_) {
      read_callback_(shared_from_this(), &read_buffer_);
    }
  }
}

void TcpConnection::WriteToPeer() {
  size_t bytes;
  auto buffer = write_buffer_.Read(&bytes);

  while (bytes != 0) {
    // TODO: 使用writev调整Buffer写入策略，同时快速递送消息
    ssize_t nbytes = ::write(fd_, buffer, bytes);
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
    buffer += nbytes;
    DLOG_INFO << "Write " << nbytes << " bytes to " << *peer_addr_;
  }

  if (bytes == 0) {
    channel_->DisableWriting();
    if (write_done_callback_) {
      write_done_callback_(shared_from_this());
    }
  }
}

void TcpConnection::HandleError() {
  int err = SocketOps::GetAndClearError(fd_);
  char buf[128];
  if (err == ECONNRESET || err == ECONNREFUSED) {
    // 不把这个当做是异常
    LOG_INFO << ::strerror_r(err, buf, sizeof(buf))
             << ", peer_addr_ = " << *peer_addr_;
  } else {
    LOG_WARNING_IF(err) << ::strerror_r(err, buf, sizeof(buf))
                        << ", peer_addr_ = " << *peer_addr_;
  }
}

size_t TcpConnection::BytesCanWrite() const {
  return write_buffer_.SpaceBeforeFull();
}

void TcpConnection::SetPeerAddr(const alpha::NetAddress& addr) {
  DCHECK(!peer_addr_);
  peer_addr_.reset(new NetAddress(addr));
}

void TcpConnection::Init() {
  channel_->set_read_callback(std::bind(&TcpConnection::ReadFromPeer, this));
  channel_->set_write_callback(std::bind(&TcpConnection::WriteToPeer, this));
  channel_->set_error_callback(std::bind(&TcpConnection::HandleError, this));
  channel_->EnableReading();
  // Try to get both sides address
  NetAddress addr;
  LocalAddr(&addr);
  PeerAddr(&addr);
}

NetAddress TcpConnection::LocalAddr() {
  NetAddress addr;
  if (LocalAddr(&addr)) {
    return addr;
  }
  return NetAddress("0.0.0.0", 0);
}

NetAddress TcpConnection::PeerAddr() {
  NetAddress addr;
  if (PeerAddr(&addr)) {
    return addr;
  }
  return NetAddress("0.0.0.0", 0);
}

bool TcpConnection::LocalAddr(NetAddress* addr) {
  if (!local_addr_) {
    bool ok = NetAddress::GetLocalAddr(fd_, addr);
    if (!ok) {
      return false;
    }
    local_addr_ = alpha::make_unique<NetAddress>(*addr);
  } else {
    *addr = *local_addr_;
  }
  return true;
}

bool TcpConnection::PeerAddr(NetAddress* addr) {
  if (!peer_addr_) {
    bool ok = NetAddress::GetPeerAddr(fd_, addr);
    if (!ok) {
      return false;
    }
    peer_addr_ = alpha::make_unique<NetAddress>(*addr);
  } else {
    *addr = *peer_addr_;
  }
  return true;
}
}
