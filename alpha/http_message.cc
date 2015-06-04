/*
 * ==============================================================================
 *
 *       Filename:  http_message.cc
 *        Created:  05/24/15 15:58:26
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */
#include "http_message.h"
#include <ctime>

namespace alpha {
    std::string HTTPMessage::FormatDate(alpha::TimeStamp time) {
        static const int kDateSize = 128;
        char buf[kDateSize];
        struct tm result;
        time_t t = time / alpha::kMilliSecondsPerSecond;
        strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S %Z", gmtime_r(&t, &result));
        return buf;
    }

    HTTPMessage::HTTPMessage() {
        fields_.which = 0;
        fields_.ptr.request = nullptr;
    }

    HTTPMessage::~HTTPMessage() {
        if (fields_.which == 1) {
            delete fields_.ptr.request;
        } else if (fields_.which == 2) {
            delete fields_.ptr.response;
        } else {
            assert (fields_.ptr.request == nullptr);
        }
    }

    HTTPHeaders& HTTPMessage::Headers() {
        return headers_;
    }

    const HTTPHeaders& HTTPMessage::Headers() const {
        return headers_;
    }

    void HTTPMessage::SetClientAddress(const alpha::NetAddress& addr) {
        request().client_ip = addr.ip();
        request().client_port = addr.port();
    }

    void HTTPMessage::SetMethod(alpha::Slice method) {
        request().method = method.ToString();
    }

    void HTTPMessage::SetPath(alpha::Slice path) {
        request().path = path.ToString();
    }

    void HTTPMessage::SetQueryString(alpha::Slice query_string) {
        request().query_string = query_string.ToString();
    }

    void HTTPMessage::SetStatus(uint16_t status) {
        response().status = status;
    }

    void HTTPMessage::SetStatusString(alpha::Slice status_string) {
        response().status_string = status_string.ToString();
    }

    void HTTPMessage::AppendBody(alpha::Slice data) {
        body_.append(data.data(), data.size());
    }

    std::string HTTPMessage::ClientIp() const {
        return request().client_ip;
    }

    uint16_t HTTPMessage::ClientPort() const {
        return request().client_port;
    }

    std::string HTTPMessage::Method() const {
        return request().method;
    }

    std::string HTTPMessage::Path() const {
        return request().path;
    }

    std::string HTTPMessage::QueryString() const {
        return request().query_string;
    }

    uint16_t HTTPMessage::Status() const {
        return response().status;
    }

    const std::string& HTTPMessage::StatusString() const {
        return response().status_string;
    }

    const std::string& HTTPMessage::Body() const {
        return body_;
    }

    const std::map<std::string, std::string> HTTPMessage::Params() const {
        if (!parsed_params_) {
            ParseParams();
        }
        return query_params_;
    }

    HTTPMessage::Request& HTTPMessage::request() {
        assert (fields_.which == 0 || fields_.which == 1);
        if (fields_.ptr.request == nullptr) {
            fields_.ptr.request = new Request;
            fields_.which = 1;
        }
        return *fields_.ptr.request;
    }

    const HTTPMessage::Request& HTTPMessage::request() const {
        return const_cast<HTTPMessage*>(this)->request();
    }

    HTTPMessage::Response& HTTPMessage::response() {
        assert (fields_.which == 0 || fields_.which == 2);
        if (fields_.ptr.response == nullptr) {
            fields_.ptr.response = new Response;
            fields_.which = 2;
        }
        return *fields_.ptr.response;
    }

    const HTTPMessage::Response& HTTPMessage::response() const {
        return const_cast<HTTPMessage*>(this)->response();
    }

    void HTTPMessage::ParseParams() const {
        alpha::Slice query_string = request().query_string;
        while (!query_string.empty()) {
            auto pos = query_string.find("=");
            if (pos == alpha::Slice::npos) {
                break;
            }
            auto key = query_string.subslice(0, pos);
            query_string.Advance(pos + 1);
            pos = query_string.find("&");
            alpha::Slice val;
            if (pos == alpha::Slice::npos) {
                val = query_string;
                query_string.Clear();
            } else {
                val = query_string.subslice(0, pos);
                query_string.Advance(pos + 1);
            }
            query_params_.emplace(key.ToString(), val.ToString());
        }
        parsed_params_ = true;
    }
}
