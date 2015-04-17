/*
 * =============================================================================
 *
 *       Filename:  tcp_client.h
 *        Created:  04/06/15 15:42:58
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#ifndef  __TCP_CLIENT_H__
#define  __TCP_CLIENT_H__

#include "compiler.h"
#include "tcp_connection.h"
#include <map>

namespace alpha {
    class Channel;
    class EventLoop;
    class NetAddress;
    class TcpConnector;
    class TcpClient {
        public:
            using ConnectedCallback = std::function<void(TcpConnectionPtr)>;
            using CloseCallback = std::function<void(TcpConnectionPtr)>;
            using ConnectErrorCallback = std::function<void(const NetAddress&)>;

            TcpClient(EventLoop* loop);
            ~TcpClient();
            DISABLE_COPY_ASSIGNMENT(TcpClient);

            void ConnectTo(const NetAddress& addr);
            void SetOnConnected(const ConnectedCallback& cb) {
                connected_callback_ = cb;
            }
            void SetOnClose(const CloseCallback& cb) {
                close_callback_ = cb;
            }
            void SetOnConnectError(const ConnectErrorCallback& cb) {
                connect_error_callback_ = cb;
            }
        private:
            void OnConnected(int fd);
            void OnClose(int fd);
            void OnConnectError(const NetAddress&);

            using TcpConnectionMap = std::map<int, TcpConnectionPtr>;
            EventLoop* loop_;
            TcpConnectionMap connections_;
            std::unique_ptr<TcpConnector> connector_;
            ConnectedCallback connected_callback_;
            CloseCallback close_callback_;
            ConnectErrorCallback connect_error_callback_;
    };
}

#endif   /* ----- #ifndef __TCP_CLIENT_H__  ----- */
