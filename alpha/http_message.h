/*
 * ==============================================================================
 *
 *       Filename:  http_message.h
 *        Created:  05/24/15 15:55:15
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#ifndef  __HTTP_MESSAGE_H__
#define  __HTTP_MESSAGE_H__

#include <map>
#include <alpha/slice.h>
#include <alpha/net_address.h>
#include <alpha/http_headers.h>

namespace alpha {
    class HTTPMessage {
        public:
            HTTPMessage();
            ~HTTPMessage();
            HTTPHeaders& Headers();
            const HTTPHeaders& Headers() const;

            void SetClientAddress(const alpha::NetAddress& addr);
            void SetMethod(alpha::Slice method);
            void SetPath(alpha::Slice path);
            void SetQueryString(alpha::Slice query_string);
            void SetStatus(uint16_t status);
            void AppendBody(alpha::Slice body);

            std::string ClientIp() const;
            uint16_t ClientPort() const;
            std::string Method() const;
            std::string Path() const;
            std::string QueryString() const;
            uint16_t Status() const;
            const std::string& Body() const;
            const std::map<std::string, std::string> Params() const;

        private:
            struct Request {
                std::string client_ip;
                int client_port;
                std::string method;
                std::string path;
                std::string query_string;
            };

            struct Response {
                uint16_t status;
                std::string status_string;
            };

            struct Fields {
                int which;
                union {
                    Request* request;
                    Response* response;
                } ptr;
            };

            Request& request();
            const Request& request() const;
            Response& response();
            const Response& response() const;
            void ParseParams() const;

            HTTPHeaders headers_;
            mutable Fields fields_;
            mutable bool parsed_params_ = false;
            mutable std::map<std::string, std::string> query_params_;
            std::string body_;
    };
}

#endif   /* ----- #ifndef __HTTP_MESSAGE_H__  ----- */
