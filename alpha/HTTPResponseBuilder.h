/*
 * =============================================================================
 *
 *       Filename:  HTTPResponseBuilder.h
 *        Created:  05/29/15 10:11:03
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include <alpha/slice.h>
#include <alpha/tcp_connection.h>
#include <alpha/HTTPMessage.h>

namespace alpha {
class HTTPResponseBuilder {
 public:
  HTTPResponseBuilder(TcpConnectionPtr& conn);

  HTTPResponseBuilder& status(int16_t code, Slice msg);
  HTTPResponseBuilder& body(Slice body);
  HTTPResponseBuilder& body(std::string&& body);

  HTTPResponseBuilder& AddHeader(Slice name, Slice value);
  void SendWithEOM();

 private:
  TcpConnectionPtr conn_;
  HTTPMessage message_;
  std::string body_;
};
}
