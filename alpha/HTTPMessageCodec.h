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
#include <limits>
#include <string>
#include <alpha/Slice.h>
#include <alpha/TcpConnection.h>
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
  std::string data_to_peer() const { return data_to_peer_; }
  void clear_data_to_peer() { data_to_peer_.clear(); }

 private:
  Status ParseStartLine(Slice& data);
  Status ParseHeader(Slice& data);
  Status OperationAfterParseHeader();
  Status AppendData(Slice& data);
  Status status_{Status::kParseStartLine};
  void ParseHTTPMessagePayload(HTTPMessage* message,
                               const std::string& boundary) const;
  uint32_t content_length_{std::numeric_limits<uint32_t>::max()};
  std::string data_to_peer_;
  HTTPMessage http_message_;
};
}  // namespace alpha
