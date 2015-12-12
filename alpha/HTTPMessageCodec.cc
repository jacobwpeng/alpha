/*
 * ==============================================================================
 *
 *       Filename:  HTTPMessageCodec.cc
 *        Created:  05/03/15 19:53:30
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#include "HTTPMessageCodec.h"
#include "logger.h"

namespace alpha {
static const Slice CRLF("\r\n");
static const Slice DoubleCRLF("\r\n\r\n");
HTTPMessageCodec::Status HTTPMessageCodec::Process(Slice& data) {
  if (status_ < 0) {
    return status_;
  }

  if (status_ == Status::kParseStartLine) {
    status_ = ParseStartLine(data);
    if (status_ < 0) {
      return status_;
    }
    DLOG_INFO << "StartLine parsed";
  }

  if (status_ == Status::kParseHeader) {
    status_ = ParseHeader(data);
    if (status_ < 0) {
      return status_;
    }
    DLOG_INFO << "HTTPHeader parsed";
  }

  if (status_ == Status::kParseData) {
    status_ = AppendData(data);
    if (status_ == Status::kDone) {
      DLOG_INFO << "Received all data";
    }
  }
  return status_;
}

HTTPMessage& HTTPMessageCodec::Done() {
  assert(status_ == Status::kDone);
  return http_message_;
}

HTTPMessageCodec::Status HTTPMessageCodec::ParseStartLine(Slice& data) {
  auto pos = data.find(CRLF);
  if (pos == Slice::npos) {
    return Status::kParseStartLine;
    ;
  }
  auto start_line = data.subslice(0, pos);
  const auto kVersionLength = Slice(" HTTP/1.0").size();
  if (!start_line.EndsWith(" HTTP/1.0") && !start_line.EndsWith(" HTTP/1.1")) {
    return Status::kInvalidHTTPVersion;
  }
  size_t method_size = 0;
  if (start_line.StartsWith("GET /")) {
    http_message_.SetMethod("GET");
    method_size = 4;
  } else if (start_line.StartsWith("PUT /")) {
    http_message_.SetMethod("PUT");
    method_size = 4;
  } else if (start_line.StartsWith("POST /")) {
    http_message_.SetMethod("POST");
    method_size = 5;
  } else if (start_line.StartsWith("DELETE /")) {
    http_message_.SetMethod("DELETE");
    method_size = 7;
  } else {
    return Status::kInvalidMethod;
  }
  start_line.Advance(method_size);
  start_line.Subtract(kVersionLength);
  auto path = start_line.ToString();
  auto query_string_start = path.find('?');
  if (query_string_start == std::string::npos) {
    http_message_.SetPath(path);
  } else {
    http_message_.SetPath(path.substr(0, query_string_start));
    http_message_.SetQueryString(path.substr(query_string_start + 1));
  }
  data.Advance(pos + CRLF.size());
  return Status::kParseHeader;
}

HTTPMessageCodec::Status HTTPMessageCodec::ParseHeader(Slice& data) {
  auto pos = data.find(CRLF);
  while (pos != Slice::npos) {
    if (pos == 0) {
      data.Advance(CRLF.size());
      return OperationAfterParseHeader();
    }
    auto line = data.subslice(0, pos);
    auto sp_pos = line.find(": ");
    if (sp_pos == 0 || sp_pos == Slice::npos) {
      return Status::kInvalidHeadLine;
    }
    auto key = line.subslice(0, sp_pos);
    auto val = line.subslice(key.size() + 2);
    http_message_.Headers().Add(key, val);
    data.Advance(line.size() + CRLF.size());
    pos = data.find(CRLF);
  }
  return Status::kParseHeader;
}

HTTPMessageCodec::Status HTTPMessageCodec::OperationAfterParseHeader() {
  return http_message_.Headers().Exists("Content-Length") ? Status::kParseData
                                                          : Status::kDone;
}

HTTPMessageCodec::Status HTTPMessageCodec::AppendData(Slice& data) {
  if (content_length_ == std::numeric_limits<uint32_t>::max()) {
    const size_t kMaxContentLengthDigitNum = 12;
    const uint32_t kMaxContentLength = 1 << 20;
    assert(http_message_.Headers().Exists("Content-Length"));
    auto content_length_str = http_message_.Headers().Get("Content-Length");
    if (content_length_str.size() >= kMaxContentLengthDigitNum) {
      return Status::kInvalidContentLength;
    }

    auto n = ::sscanf(content_length_str.data(), "%u", &content_length_);
    if (n != 1 || content_length_ > kMaxContentLength) {
      return Status::kInvalidContentLength;
    }
    DLOG_INFO << "Content length: " << content_length_;
  }

  assert(content_length_ > 0);
  const auto current_body_size = http_message_.Body().size();
  CHECK(current_body_size < content_length_);
  auto content_length_to_append =
      std::min(data.size(), content_length_ - current_body_size);
  http_message_.AppendBody(data.subslice(0, content_length_to_append));
  data.Advance(content_length_to_append);
  return http_message_.Body().size() == content_length_ ? Status::kDone
                                                        : Status::kParseData;
}
}
