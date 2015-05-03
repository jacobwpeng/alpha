/*
 * ==============================================================================
 *
 *       Filename:  simple_http_server.cc
 *        Created:  05/03/15 19:16:30
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#include "simple_http_server.h"
#include "compiler.h"
#include "logger.h"
#include "format.h"
#include "net_address.h"
#include "event_loop.h"
#include "tcp_server.h"
#include "http_message_codec.h"

namespace alpha {
    SimpleHTTPServer::SimpleHTTPServer(EventLoop* loop, const alpha::NetAddress& addr)
        :loop_(loop), addr_(addr) {
    }

    SimpleHTTPServer::~SimpleHTTPServer() = default;

    bool SimpleHTTPServer::Run() {
        using namespace std::placeholders;
        server_.reset(new TcpServer(loop_, addr_));
        server_->SetOnRead(std::bind(&SimpleHTTPServer::OnMessage, this, _1, _2));
        server_->SetOnNewConnection(std::bind(&SimpleHTTPServer::OnConnected, this, _1));
        server_->SetOnClose(std::bind(&SimpleHTTPServer::OnClose, this, _1));
        return server_->Run();
    }

    void SimpleHTTPServer::DefaultRequestCallback(TcpConnectionPtr conn, Slice method, Slice path
            , const HTTPHeader& header, Slice data) {
        LOG_INFO << "New request from " << conn->PeerAddr();
        LOG_INFO << "method = " << method.ToString();
        LOG_INFO << "path = " << path.ToString();
        for (auto & p : header) {
            LOG_INFO << p.first << ": " << p.second;
        }
        if (!data.empty()) {
            LOG_INFO << "data = \n" << HexDump(data);
        }
        conn->Write("{\"result\": \"0\", \"msg\": \"ok\"}");
    }
    
    void SimpleHTTPServer::OnMessage(TcpConnectionPtr conn, TcpConnectionBuffer* buffer) {
        HTTPMessageCodec** p = conn->GetContext<HTTPMessageCodec*>();
        assert (p && *p);
        auto codec = *p;
        int consumed;
        auto data = buffer->Read();
        DLOG_INFO << "data = \n" << HexDump(data);
        auto status = codec->Process(data, &consumed);
        assert (consumed >= 0);
        if (status == HTTPMessageCodec::Status::kNeedsMore) {
            DLOG_INFO << "codec consume " << consumed << " bytes";
            buffer->ConsumeBytes(consumed);
        } else if (status == HTTPMessageCodec::Status::kDone) {
            auto method = codec->method();
            if (method == HTTPMessageCodec::Method::kPut && put_callback_) {
                put_callback_(conn, codec->path(), codec->headers(), codec->data());
            } else if (method == HTTPMessageCodec::Method::kPost && post_callback_) {
                post_callback_(conn, codec->path(), codec->headers(), codec->data());
            } else if (method == HTTPMessageCodec::Method::kGet && get_callback_) {
                get_callback_(conn, codec->path(), codec->headers(), codec->data());
            } else if (method == HTTPMessageCodec::Method::kDelete && delete_callback_) {
                delete_callback_(conn, codec->path(), codec->headers(), codec->data());
            } else {
                DefaultRequestCallback(conn, codec->method_name(), codec->path(), codec->headers(), codec->data());
            }
            conn->Close();
        } else {
            LOG_WARNING << "Codec error, status = " << status;
            conn->Close();
        }
    }

    void SimpleHTTPServer::OnConnected(TcpConnectionPtr conn) {
        HTTPMessageCodec* codec = new HTTPMessageCodec;
        conn->SetContext(codec);
    }

    void SimpleHTTPServer::OnClose(TcpConnectionPtr conn) {
        auto** p = conn->GetContext<HTTPMessageCodec*>();
        assert (p && *p);
        delete *p;
    }
}
