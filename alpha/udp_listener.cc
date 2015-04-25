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

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "logger.h"
#include "net_address.h"
#include "channel.h"
#include "event_loop.h"

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

    bool UdpListener::Run(const NetAddress& addr) {
        fd_ = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (unlikely(fd_ == -1)) {
            PLOG_WARNING << "Create socket failed";
            return false;
        }
        auto sa = addr.ToSockAddr();
        int err = ::bind(fd_, reinterpret_cast<const sockaddr*>(&sa), sizeof(sa));
        if (err) {
            PLOG_WARNING << "bind failed, addr = " << addr;
            return false;
        }
        channel_.reset(new Channel(loop_, fd_));
        channel_->set_read_callback(std::bind(&UdpListener::OnMessage, this));
        channel_->EnableReading();
        channel_->DisableWriting();
        return true;
    }

    void UdpListener::OnMessage() {
        const size_t kMaxUdpMessageSize = 1 << 16; //64KiB
        char in[kMaxUdpMessageSize];
        char out[kMaxUdpMessageSize];
        struct sockaddr_in ca;
        socklen_t len = sizeof(ca);
        ssize_t bytes = ::recvfrom(fd_, in, sizeof(in), MSG_DONTWAIT, 
                reinterpret_cast<struct sockaddr *>(&ca), &len);
        if (bytes < 0) {
            PLOG_WARNING << "recvfrom failed";
            return;
        }
        if (message_callback_) {
            ssize_t n = message_callback_(Slice(in, bytes), out);
            assert (n <= static_cast<ssize_t>(kMaxUdpMessageSize));
            if (n <= 0) {
                //不回复消息
                DLOG_INFO << "message_callback_ returns n = " << n;
            } else {
                ::sendto(fd_, out, n, MSG_DONTWAIT,
                        reinterpret_cast<struct sockaddr*>(&ca), sizeof(ca));
            }
        } else {
            LOG_INFO << "Receive " << bytes << " from " << NetAddress(ca);
        }
    }
}
