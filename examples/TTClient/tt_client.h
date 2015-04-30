/*
 * =============================================================================
 *
 *       Filename:  tt_client.h
 *        Created:  04/30/15 17:22:20
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#ifndef  __TT_CLIENT_H__
#define  __TT_CLIENT_H__

#include <alpha/slice.h>
#include <alpha/tcp_connection.h>

namespace alpha {
    class NetAddress;
    class Coroutine;
    class EventLoop;
    class TcpClient;
}

namespace tokyotyrant {
    class Client {
        public:
            enum Code {
                kOk = 1,
                kConn = 100,
            };
            Client(alpha::EventLoop* loop);
            void SetCoroutine(alpha::Coroutine* co);
            bool Connnect(const alpha::NetAddress& addr);
            bool Connected() const;
            //int Put(alpha::Slice key, alpha::Slice value);
            int Stat(std::string* stat);

        private:
            enum class ConnectionState {
                kConnected = 1,
                kConnecting = 2,
                kDisconnected = 3
            };
            void OnConnectError(const alpha::NetAddress& addr);
            void OnConnected(alpha::TcpConnectionPtr conn);
            void OnDisconnected(alpha::TcpConnectionPtr conn);
            void OnMessage(alpha::TcpConnectionPtr conn, alpha::TcpConnectionBuffer* buffer);
            void OnTimeout();
            bool connect_error_;
            alpha::EventLoop* loop_;
            alpha::Coroutine* co_;
            alpha::TcpConnectionPtr conn_;
            ConnectionState state_;
            std::unique_ptr<alpha::NetAddress> addr_;
            std::unique_ptr<alpha::TcpClient> tcp_client_;
    };
}

#endif   /* ----- #ifndef __TT_CLIENT_H__  ----- */
