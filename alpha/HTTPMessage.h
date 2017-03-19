/*
 * ==============================================================================
 *
 *       Filename:  HTTPMessage.h
 *        Created:  05/24/15 15:55:15
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#pragma once

#include <map>
#include <alpha/TimeUtil.h>
#include <alpha/Slice.h>
#include <alpha/NetAddress.h>
#include <alpha/HTTPHeaders.h>

namespace alpha {
class HTTPMessage {
 public:
  static std::string FormatDate(alpha::TimeStamp time);
  HTTPMessage();
  ~HTTPMessage();
  HTTPHeaders& Headers();
  const HTTPHeaders& Headers() const;

  void SetClientAddress(const alpha::NetAddress& addr);
  void SetMethod(alpha::Slice method);
  void SetPath(alpha::Slice path);
  void SetQueryString(alpha::Slice query_string);
  void SetStatus(uint16_t status);
  void SetStatusString(alpha::Slice status_string);
  void AppendBody(alpha::Slice body);
  void AddPayload(alpha::Slice payload);

  std::string ClientIp() const;
  uint16_t ClientPort() const;
  std::string Method() const;
  std::string Path() const;
  std::string QueryString() const;
  uint16_t Status() const;
  const std::string& StatusString() const;
  const std::string& Body() const;
  const std::map<std::string, std::string> Params() const;
  const std::vector<alpha::Slice> payloads() const { return payloads_; }

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
  std::vector<alpha::Slice> payloads_;
};
}
