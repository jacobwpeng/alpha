/*
 * =============================================================================
 *
 *       Filename:  http_response_builder.cc
 *        Created:  05/29/15 10:25:59
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include "http_response_builder.h"
#include <cassert>

namespace alpha {
    HTTPResponseBuilder::HTTPResponseBuilder(TcpConnectionPtr& conn)
        :conn_(conn) {
        assert (conn_ && !conn_->closed());
    }

    HTTPResponseBuilder& HTTPResponseBuilder::status(int16_t code, Slice msg) {
        message_.SetStatus(code);
        message_.SetStatusString(msg);
        return *this;
    }

    HTTPResponseBuilder& HTTPResponseBuilder::body(Slice body) {
        body_ = body.ToString();
        return *this;
    }

    HTTPResponseBuilder& HTTPResponseBuilder::body(std::string&& body) {
        body_ = std::move(body);
        return *this;
    }

    HTTPResponseBuilder& HTTPResponseBuilder::AddHeader(Slice name, Slice value) {
        message_.Headers().Add(name, value);
        return *this;
    }

    void HTTPResponseBuilder::SendWithEOM() {
        char buf[64];
        const char* CRLF = "\r\n";
        auto nbytes = snprintf(buf, sizeof(buf), "HTTP/1.0 %d %s%s",
                    message_.Status(), message_.StatusString().c_str(), CRLF);
        assert (nbytes < static_cast<ssize_t>(sizeof(buf)));
        conn_->Write(alpha::Slice(buf, nbytes));
        message_.Headers().Foreach([&CRLF, this]
                (const std::string& name, const std::string& val) {
            conn_->Write(name);
            conn_->Write(": ");
            conn_->Write(val);
            conn_->Write(CRLF);
        });
        conn_->Write("Content-Length: ");
        conn_->Write(std::to_string(body_.size()));
        conn_->Write(CRLF);
        if (!body_.empty()) {
            conn_->Write(CRLF);
            conn_->Write(body_);
        }
        conn_->Close();
    }
}
