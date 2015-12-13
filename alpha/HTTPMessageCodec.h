/*
 * ==============================================================================
 *
 *       Filename:  HTTPMessageCodec.h
 *        Created:  05/03/15 19:40:11
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#pragma once

#include <map>
#include <string>
#include <alpha/slice.h>
#include <alpha/tcp_connection.h>
#include <alpha/HTTPMessage.h>

namespace alpha {
class HTTPMessageCodec {
 public:
  /*
   * == 0 Done
   * < 0 Error
   * > 0 NeedsMore
   */
  enum Status {
    kDone = 0,

    kInvalidMethod = -100,
    kInvalidStartLine = -101,
    kInvalidHTTPVersion = -102,
    kInvalidHeadLine = -103,
    kDuplicatedHead = -104,
    kInvalidContentLength = -105,
    kTooMuchContent = -106,

    kParseStartLine = 100,
    kParseHeader = 101,
    kParseEmptyLine = 102,
    kParseData = 103,
  };

  using HTTPHeader = std::map<std::string, std::string>;

  Status Process(Slice& data);
  HTTPMessage& Done();
  Status status() const { return status_; }

 private:
  Status ParseStartLine(Slice& data);
  Status ParseHeader(Slice& data);
  Status OperationAfterParseHeader();
  Status AppendData(Slice& data);
  Status status_{Status::kParseStartLine};
  uint32_t content_length_{std::numeric_limits<uint32_t>::max()};
  HTTPMessage http_message_;
};
}
