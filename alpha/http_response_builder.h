/*
 * =============================================================================
 *
 *       Filename:  http_response_builder.h
 *        Created:  05/29/15 10:11:03
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __HTTP_RESPONSE_BUILDER_H__
#define __HTTP_RESPONSE_BUILDER_H__

#include <alpha/slice.h>
#include <alpha/tcp_connection.h>
#include <alpha/http_message.h>

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

#endif /* ----- #ifndef __HTTP_RESPONSE_BUILDER_H__  ----- */
