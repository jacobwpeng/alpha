/*
 * =============================================================================
 *
 *       Filename:  tcp_connector.h
 *        Created:  04/06/15 15:18:03
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#ifndef  __TCP_CONNECTOR_H__
#define  __TCP_CONNECTOR_H__

#include <functional>

namespace alpha {
    class EventLoop;
    class NetAddress;
    class TcpConnector {
        public:
            using ConnectCallback = std::function<void(int, bool)>;
            using ErrorCallback = std::function<void(const NetAddress&)>;

            TcpConnector(EventLoop* loop);

            void SetOnConnect(const ConnectCallback& cb) {
                connect_callback_ = cb;
            }
            void SetOnError(const ErrorCallback& cb) {
                error_callback_ = cb;
            }

            void ConnectTo(const NetAddress& addr);

        private:
            EventLoop* loop_;
            ConnectCallback connect_callback_;
            ErrorCallback error_callback_;
    };
}

#endif   /* ----- #ifndef __TCP_CONNECTOR_H__  ----- */
