/*
 * =====================================================================================
 *
 *       Filename:  udp_server.h
 *        Created:  08/22/14 15:23:16
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =====================================================================================
 */

#ifndef  __UDP_SERVER_H__
#define  __UDP_SERVER_H__

#include <string>
#include <memory>
#include <functional>
#include "compiler.h"
#include "net_address.h"
#include "slice.h"
#include "udp_listener.h"

namespace alpha {
    class Buffer;
    class EventLoop;
    class UdpListener;
    class NetAddress;

    class UdpServer {
        public:
            using MessageCallback = UdpListener::MessageCallback;

        public:
            UdpServer(EventLoop * loop);
            ~UdpServer();
            DISABLE_COPY_ASSIGNMENT(UdpServer);

            bool Start(const NetAddress& addr, const MessageCallback& cb);

        private:
            EventLoop * loop_;
            std::unique_ptr<UdpListener> listener_;
    };
}

#endif   /* ----- #ifndef __UDP_SERVER_H__  ----- */
