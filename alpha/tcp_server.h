/*
 * =============================================================================
 *
 *       Filename:  tcp_server.h
 *        Created:  04/05/15 15:32:25
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#ifndef  __TCP_SERVER_H__
#define  __TCP_SERVER_H__

#include <map>
#include <memory>
#include <functional>
#include "tcp_connection.h"

namespace alpha {
    class NetAddress;
    class TcpConnectionBuffer;
    class TcpAcceptor;
    class EventLoop;

    class TcpServer {
        public:
            using ReadCallback = 
                std::function<void(TcpConnectionPtr, TcpConnectionBuffer*)>;
            using CloseCallback = std::function<void(TcpConnectionPtr)>;
            using NewConnectionCallback = std::function<void(TcpConnectionPtr)>;

        public:
            TcpServer(EventLoop* loop, const NetAddress& addr);
            ~TcpServer();
            TcpServer(TcpServer&&) = delete;
            TcpServer(const TcpServer&) = delete;

            bool Run();
            void SetOnRead(const ReadCallback& cb) {
                read_callback_ = cb;
            }
            void SetOnClose(const CloseCallback& cb) {
                close_callback_ = cb;
            }
            void SetOnNewConnection(const NewConnectionCallback& cb) {
                new_connection_callback_ = cb;
            }

        private:
            void OnNewConnection(int fd);
            void OnConnectionClose(int fd);
            using TcpConnectionMap = std::map<int, TcpConnectionPtr>;

            EventLoop * loop_;
            NetAddress listen_addr_;
            std::unique_ptr<TcpAcceptor> acceptor_;
            ReadCallback read_callback_;
            CloseCallback close_callback_;
            NewConnectionCallback new_connection_callback_;
            TcpConnectionMap connections_;
    };
}

#endif   /* ----- #ifndef __TCP_SERVER_H__  ----- */
