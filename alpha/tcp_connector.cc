/*
 * =============================================================================
 *
 *       Filename:  tcp_connector.cc
 *        Created:  04/06/15 15:31:00
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include "tcp_connector.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <cassert>

#include "compiler.h"
#include "logger.h"
#include "event_loop.h"
#include "net_address.h"
#include "socket_ops.h"

static void DelayChannelDestroy(std::shared_ptr<alpha::Channel> channel) {
}

namespace alpha {
    TcpConnector::TcpConnector(EventLoop* loop)
        :loop_(loop) {
    }

    TcpConnector::~TcpConnector() {
        for (auto & p : connecting_fds_) {
            ::close(p.first);
        }
    }

    void TcpConnector::ConnectTo(const alpha::NetAddress& addr) {
        assert (connected_callback_);
        int fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (unlikely(fd == -1)) {
            PLOG_ERROR << "Create socket failed, addr = " << addr;
            assert (false);
            return;
        }
        SocketOps::SetNonBlocking(fd);
        struct sockaddr_in sock_addr = addr.ToSockAddr();
        int err = ::connect(fd, reinterpret_cast<sockaddr*>(&sock_addr),
                sizeof(sock_addr));
        if (err) {
            if (errno == EINPROGRESS) {
                DLOG_INFO << "connect inprogress to " << addr;
                bool ok = AddConnectingFd(fd, addr);
                assert(ok);
                (void) ok;
            } else {
                ::close(fd);
                PLOG_WARNING << "connect to " << addr << ", failed";
                if (error_callback_) {
                    error_callback_(addr);
                }
            }
        } else {
            DLOG_INFO << "connected to " << addr;
            connected_callback_(fd);
        }
    }

    void TcpConnector::OnConnected(int fd) {
        bool ok = RemoveConnectingFd(fd);
        assert (ok);
        (void) ok;
        connected_callback_(fd);
    }

    void TcpConnector::OnError(int fd, const alpha::NetAddress& addr) {
        int err = SocketOps::GetAndClearError(fd);
        bool ok = RemoveConnectingFd(fd);
        assert (ok);
        (void) ok;
        char buf[128];
        char* msg = ::strerror_r(err, buf, sizeof(buf));
        if (err == ECONNREFUSED) {
            // 不把这个当做是异常
            LOG_INFO << msg << ", addr = " << addr;
        } else {
            LOG_WARNING << msg << ", addr = " << addr;
        }
        ::close(fd);
        if (error_callback_) {
            error_callback_(addr);
        }
    }

    bool TcpConnector::RemoveConnectingFd(int fd) {
        auto it = connecting_fds_.find(fd);
        if (it == connecting_fds_.end()) {
            return false;
        } 
        std::shared_ptr<Channel> channel(std::move(it->second));
        channel->set_write_callback(nullptr);
        channel->Remove();
        loop_->QueueInLoop(std::bind(DelayChannelDestroy, channel));
        connecting_fds_.erase(it);
        return true;
    }

    bool TcpConnector::AddConnectingFd(int fd, const NetAddress& addr) {
        auto it = connecting_fds_.find(fd);
        if (it != connecting_fds_.end()) {
            return false;
        }
        std::unique_ptr<Channel> channel(new Channel(loop_, fd));
        using namespace std::placeholders;
        DLOG_INFO << "Create channel for Connect fd, channel = " << channel.get();
        channel->set_error_callback(std::bind(&TcpConnector::OnError, this, fd, addr));
        channel->set_write_callback(std::bind(&TcpConnector::OnConnected, this, fd));
        channel->EnableWriting();
        connecting_fds_.emplace(fd, std::move(channel));
        return true;
    }
}
