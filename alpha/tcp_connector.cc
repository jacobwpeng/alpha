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
#include <cassert>

#include "compiler.h"
#include "logger.h"
#include "event_loop.h"
#include "net_address.h"
#include "socket_ops.h"

namespace alpha {
    TcpConnector::TcpConnector(EventLoop* loop)
        :loop_(loop) {
    }

    void TcpConnector::ConnectTo(const alpha::NetAddress& addr) {
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
                assert (connect_callback_);
                DLOG_INFO << "connect inprogress to " << addr;
                connect_callback_(fd, false);
            } else {
                ::close(fd);
                PLOG_WARNING << "connect to " << addr << ", failed";
                if (error_callback_) {
                    error_callback_(addr);
                }
            }
        } else {
            assert (connect_callback_);
            DLOG_INFO << "connected to " << addr;
            connect_callback_(fd, true);
        }
    }
}
