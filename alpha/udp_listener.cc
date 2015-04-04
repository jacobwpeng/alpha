/*
 * =====================================================================================
 *
 *       Filename:  udp_listener.cc
 *        Created:  08/22/14 15:47:38
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =====================================================================================
 */

#include "udp_listener.h"
#include "channel.h"
#include "event_loop.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <boost/bind.hpp>
#include "logger.h"

namespace alpha {
    UdpListener::UdpListener(EventLoop * loop)
        :loop_(loop), fd_(-1) {
    }

    UdpListener::~UdpListener() {
        if (channel_) {
            channel_->Remove();
            ::close(fd_);
        }
    }

    void UdpListener::BindOrAbort(const std::string& ip, int port) {
        fd_ = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        assert (fd_ >= 0);
        //PCHECK(fd_ >= 0) << "create socket failed";

        struct sockaddr_in sa;

        memset(&sa, 0x0, sizeof(sa));

        sa.sin_family = AF_INET;
        int ret = ::inet_pton(AF_INET, ip.c_str(), &sa.sin_addr);
        assert (ret == 1);
        //PCHECK(ret == 1) << "inet_pton failed, ip[" << ip << "], port[" << port << "]";

        sa.sin_port = htons(port);
        ret = bind(fd_, reinterpret_cast<const struct sockaddr *>(&sa), sizeof(sa));
        assert (ret == 0);
        //PCHECK(ret == 0) << "bind failed";
    }

    void UdpListener::Start() {
        channel_.reset(new Channel(loop_, fd_));
        channel_->set_read_callback(boost::bind(&UdpListener::OnMessage, this));
        channel_->EnableReading();
        channel_->DisableWriting();
    }

    void UdpListener::OnMessage() {
        const size_t kMaxUdpMessageSize = 1 << 16; //64KiB
        char in[kMaxUdpMessageSize];
        struct sockaddr_in ca;
        socklen_t len = sizeof(ca);
        ssize_t bytes = ::recvfrom(fd_, in, sizeof(in), MSG_DONTWAIT, 
                reinterpret_cast<struct sockaddr *>(&ca), &len);
        if (bytes < 0) {
            PLOG_WARNING << "recvfrom failed";
            return;
        }
        if (mcb_) {
            std::string out;
            int ret = mcb_(in, bytes, &out);
            if (ret < 0) {
                LOG_WARNING << "MessageCallback return " << ret;
            }
            else if (out.size() > kMaxUdpMessageSize) {
                LOG_WARNING << "reply size[" << out.size() 
                    << "] is too larger, truncate reply to 64KiB";
                ::sendto(fd_, out.data(), kMaxUdpMessageSize, MSG_DONTWAIT, 
                        reinterpret_cast<struct sockaddr *>(&ca), sizeof(ca));
            }
            else {
                ::sendto(fd_, out.data(), out.size(), MSG_DONTWAIT, 
                        reinterpret_cast<struct sockaddr *>(&ca), sizeof(ca));
            }
        }
    }
}
