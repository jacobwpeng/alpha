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
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>

namespace alpha {
    class Buffer;
    class EventLoop;
    class UdpListener;

    class UdpServer : boost::noncopyable {
        public:
            typedef boost::function< int(const char*, size_t, std::string*) > ReadCallback;

        public:
            UdpServer(EventLoop * loop, const std::string& ip, int port);
            ~UdpServer();

            void Start();
            void set_read_callback(const ReadCallback& rcb) { rcb_ = rcb; }

        private:
            EventLoop * loop_;
            ReadCallback rcb_;
            boost::scoped_ptr<UdpListener> listener_;

            const std::string ip_;
            const int port_;
    };
}

#endif   /* ----- #ifndef __UDP_SERVER_H__  ----- */
