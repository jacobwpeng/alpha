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
  auto content_type = http_message_.Headers().Get("Content-Type");
  DLOG_INFO << alpha::Slice(content_type).ToString();
  if (http_message_.Method() == "POST" &&
      alpha::Slice(content_type).StartsWith("multipart/form-data;")) {
    alpha::Slice key = "boundary=";
    auto pos = content_type.find(key.data());
    if (pos != std::string::npos) {
      auto boundary = content_type.substr(pos + key.size());
      ParseHTTPMessagePayload(&http_message_, boundary);
    }
  }
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

  if (content_length_ == 0) {
    return Status::kDone;
  }
  const auto current_body_size = http_message_.Body().size();
  CHECK(current_body_size < content_length_);
  auto content_length_to_append =
      std::min(data.size(), content_length_ - current_body_size);
  http_message_.AppendBody(data.subslice(0, content_length_to_append));
  data.Advance(content_length_to_append);
  return http_message_.Body().size() == content_length_ ? Status::kDone
                                                        : Status::kParseData;
}

void HTTPMessageCodec::ParseHTTPMessagePayload(
    HTTPMessage* message, const std::string& boundary) const {
  DLOG_INFO << "Boundary: " << boundary;
  std::string boundary_sep = "--" + boundary;
  std::string boundary_end = "--" + boundary + "--";
  auto body = alpha::Slice(message->Body());
  // boundary_sep
  // Content-Info
  // CRLF
  // Payload
  // boundary_sep
  // Content-Info
  // CRLF
  // ...
  // ...
  // ...
  // boundary_end
  while (!body.empty()) {
    body.Advance(boundary_sep.size() + CRLF.size());
    auto pos = body.find(CRLF);
    alpha::Slice payload_name;
    while (pos) {
      // Still in Content-Info
      if (body.StartsWith("Content-Disposition")) {
        // Parse name
        //  auto name_start = alpha::Slice("name=\"");
        //  auto name_start_pos = body.find(name_start);
        //  CHECK(name_start_pos != alpha::Slice::npos);
        //  CHECK(name_start_pos < pos);
        //  payload_name = body.subslice(name_start_pos + name_start.size(),
        //                               pos - 1 - name_start_pos);
        //  DLOG_INFO << "Payload name: " << payload_name.ToString();
      }
      body.Advance(CRLF.size() + pos);
      pos = body.find(CRLF);
    }
    // CHECK(!payload_name.empty());
    body.Advance(CRLF.size());
    // Now payload
    auto payload_end = body.find(boundary_sep);
    auto payload = body.subslice(0, payload_end - 2);
    DLOG_INFO << "Payload size: " << payload.size();
    message->AddPayload(payload);
    if (body.find(boundary_end) == payload_end) {
      break;
    }
  }
}
}
