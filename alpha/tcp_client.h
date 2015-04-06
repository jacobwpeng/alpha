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
    class EventLoop;
    class NetAddress;
    class TcpConnector;
    class TcpClient {
        public:
            using ConnectedCallback = TcpConnection::ConnectedCallback;
            using ReadCallback = TcpConnection::ReadCallback;
            using CloseCallback = std::function<void(TcpConnectionPtr)>;

            TcpClient(EventLoop* loop);
            ~TcpClient();
            DISABLE_COPY_ASSIGNMENT(TcpClient);

            void ConnectTo(const NetAddress& addr);
            void SetOnConnected(const ConnectedCallback& cb) {
                connected_callback_ = cb;
            }
            void SetOnRead(const ReadCallback& cb) {
                read_callback_ = cb;
            }
            void SetOnClose(const CloseCallback& cb) {
                close_callback_ = cb;
            }

        private:
            void OnConnect(int fd, bool connected);
            void OnConnected(TcpConnectionPtr conn);
            void OnConnectError(const NetAddress& addr);
            void OnClose(int fd);

            using TcpConnectionMap = std::map<int, TcpConnectionPtr>;
            EventLoop* loop_;
            TcpConnectionMap connections_;
            std::unique_ptr<TcpConnector> connector_;
            ConnectedCallback connected_callback_;
            ReadCallback read_callback_;
            CloseCallback close_callback_;
    };
}

#endif   /* ----- #ifndef __TCP_CLIENT_H__  ----- */
