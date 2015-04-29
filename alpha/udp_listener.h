/*
 * =====================================================================================
 *
 *       Filename:  udp_listener.h
 *        Created:  08/22/14 15:44:10
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  listen to udp message
 *
 * =====================================================================================
 */

#ifndef  __UDP_LISTENER_H__
#define  __UDP_LISTENER_H__

#include <string>
#include <memory>
#include <functional>
#include "compiler.h"
#include "slice.h"

namespace alpha {
    class Channel;
    class EventLoop;
    class NetAddress;
    class UdpListener {
        public:
            static const size_t kOutputBufferSize = 1 << 16; //64KiB
            using MessageCallback = std::function<ssize_t (Slice data, char* out)>;;

        public:
            UdpListener(EventLoop * loop);
            ~UdpListener();
            DISABLE_COPY_ASSIGNMENT(UdpListener);

            void set_message_callback(const MessageCallback& cb) { message_callback_ = cb; }
            bool Run(const NetAddress& addr);

        private:
            void OnMessage();

        private:
            EventLoop * loop_;
            int fd_;
            std::unique_ptr<Channel> channel_;
            MessageCallback message_callback_;
    };
}
#endif   /* ----- #ifndef __UDP_LISTENER_H__  ----- */
