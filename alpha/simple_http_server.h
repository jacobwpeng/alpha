/*
 * ==============================================================================
 *
 *       Filename:  simple_http_server.h
 *        Created:  05/03/15 19:11:14
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  Simple HTTP/1.0 server implementation
 *
 * ==============================================================================
 */
#ifndef  __SIMPLE_HTTP_SERVER_H__
#define  __SIMPLE_HTTP_SERVER_H__

#include <map>
#include "slice.h"
#include "net_address.h"
#include "tcp_connection.h"
#include "http_message_codec.h"

namespace alpha {
    class EventLoop;
    class TcpServer;
    class NetAddress;
    class SimpleHTTPServer {
        public:
            using HTTPHeader = HTTPMessageCodec::HTTPHeader;
            using RequestCallback = std::function<
                void(TcpConnectionPtr, Slice path, const HTTPHeader& header, Slice data)>;
            SimpleHTTPServer(EventLoop* loop, const NetAddress& addr);
            ~SimpleHTTPServer();
            bool Run();
            void SetOnGet(const RequestCallback& cb) { get_callback_ = cb; }
            void SetOnPut(const RequestCallback& cb) { put_callback_ = cb; }
            void SetOnPost(const RequestCallback& cb) { post_callback_ = cb; }
            void SetOnDelete(const RequestCallback& cb) { delete_callback_ = cb; }

        private:
            void DefaultRequestCallback(TcpConnectionPtr, Slice method, Slice path
                    , const HTTPHeader& header, Slice data);
            void OnMessage(TcpConnectionPtr conn, TcpConnectionBuffer* buffer);
            void OnConnected(TcpConnectionPtr conn);
            void OnClose(TcpConnectionPtr conn);

            static const int kReadHeaderTimeout = 3000; //ms
            EventLoop* loop_;
            NetAddress addr_;
            std::unique_ptr<TcpServer> server_;
            RequestCallback get_callback_;
            RequestCallback put_callback_;
            RequestCallback post_callback_;
            RequestCallback delete_callback_;
    };
}

#endif   /* ----- #ifndef __SIMPLE_HTTP_SERVER_H__  ----- */
