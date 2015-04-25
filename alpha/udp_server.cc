/*
 * =====================================================================================
 *
 *       Filename:  udp_server.cc
 *        Created:  08/22/14 15:28:49
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =====================================================================================
 */

#include "udp_server.h"

#include "logger.h"
#include "net_address.h"
#include "event_loop.h"
#include "udp_listener.h"

namespace alpha {
    UdpServer::UdpServer(EventLoop * loop)
        :loop_(loop) {
    }

    UdpServer::~UdpServer() {
    }

    bool UdpServer::Start(const NetAddress& addr, const MessageCallback& cb) {
        listener_.reset(new UdpListener(loop_));
        listener_->set_message_callback(cb);
        return listener_->Run(addr);
    }
}
