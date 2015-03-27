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

#include "event_loop.h"
#include "udp_listener.h"
#include "logger.h"

namespace alpha {
    UdpServer::UdpServer(EventLoop * loop, const std::string& ip, int port)
        :loop_(loop), ip_(ip), port_(port) {
    }

    UdpServer::~UdpServer() {
    }

    void UdpServer::Start() {
        listener_.reset(new UdpListener(loop_));
        listener_->set_message_callback(rcb_);
        LOG_INFO << "bind to " << ip_ << ":" << port_;
        listener_->BindOrAbort(ip_, port_);
        listener_->Start();
    }
}
