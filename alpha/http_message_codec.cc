/*
 * ==============================================================================
 *
 *       Filename:  http_message_codec.cc
 *        Created:  05/03/15 19:53:30
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#include "http_message_codec.h"
#include "logger.h"

namespace alpha {
    static const Slice CRLF("\r\n");
    static const Slice DoubleCRLF("\r\n\r\n");
    HTTPMessageCodec::Status HTTPMessageCodec::Process(Slice slice,
            int* consumed) {
        *consumed = 0;
        if (status_ < Status::kParseStartLine) {
            return status_;
        }

        if (status_ == Status::kParseStartLine) {
            auto pos = slice.find(CRLF);
            if (pos == Slice::npos) {
                return Status::kNeedsMore;
            } else {
                auto start_line = slice.subslice(0, pos);
                status_ = ParseStartLine(start_line);
                if (status_ != Status::kParseHeader) {
                    return status_;
                }
                *consumed += pos + CRLF.size();
                slice = slice.RemovePrefix(*consumed);
            }
        }

        if (status_ == Status::kParseHeader) {
            int consumed_by_parse_header = 0;
            status_ = ParseHeader(slice, &consumed_by_parse_header);
            if (status_ != Status::kParseData && status_ != Status::kDone) {
                return status_;
            }
            *consumed += consumed_by_parse_header;
            slice = slice.RemovePrefix(consumed_by_parse_header);
        }

        if (status_ == Status::kParseData) {
            status_ = AppendData(slice);
            if (status_ != Status::kNeedsMore && status_ != Status::kDone) {
                return status_;
            } else {
                *consumed += slice.size();
            }
        }
        return status_;
    }

    HTTPMessage& HTTPMessageCodec::Done() {
        assert (status_ == Status::kDone);
        return http_message_;
    }

    HTTPMessageCodec::Status HTTPMessageCodec::ParseStartLine(Slice start_line) {
            const auto kVersionLength = Slice(" HTTP/1.0").size();
            if (!start_line.EndsWith(" HTTP/1.0")
                    && !start_line.EndsWith(" HTTP/1.1")) {
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
            }
            else if (start_line.StartsWith("DELETE /")) {
                http_message_.SetMethod("DELETE");
                method_size = 7;
            } else {
                return Status::kInvalidMethod;
            }
            auto path = start_line.RemovePrefix(method_size).RemoveSuffix(
                kVersionLength).ToString();
            http_message_.SetPath(path);
        return Status::kParseHeader;
    }

    HTTPMessageCodec::Status HTTPMessageCodec::ParseHeader(Slice data,
            int* consumed) {
        auto pos = data.find(CRLF);
        while (pos != Slice::npos) {
            if (pos == 0) {
                *consumed += 2;
                return OperationAfterParseHeader();
            }
            auto line = data.subslice(0, pos);
            auto sp_pos = line.find(": ");
            if (sp_pos == 0 || sp_pos == Slice::npos) {
                return Status::kInvalidHeadLine;
            }
            auto key = line.subslice(0, sp_pos);
            auto val = line.RemovePrefix(key.size() + 2);
            http_message_.Headers().Add(key, val);
            *consumed += line.size() + CRLF.size();
            data = data.RemovePrefix(line.size() + CRLF.size());
            pos = data.find(CRLF);
        }
        return Status::kNeedsMore;
    }

    HTTPMessageCodec::Status HTTPMessageCodec::OperationAfterParseHeader() {
        return http_message_.Headers().Exists("Content-Length")
            ? Status::kParseData : Status::kDone;
    }

    HTTPMessageCodec::Status HTTPMessageCodec::AppendData(Slice data) {
        if (content_length_ == -1) {
            const size_t kMaxContentLengthDigitNum = 12;
            const int kMaxContentLength = 1024;
            assert (http_message_.Headers().Exists("Content-Length"));
            auto content_length_str =
                http_message_.Headers().Get("Content-Length");
            if (content_length_str.size() >= kMaxContentLengthDigitNum) {
                return Status::kInvalidContentLength;
            }

            auto n = ::sscanf(content_length_str.data(), "%d", &content_length_);
            if (n != 1 || content_length_ <= 0 
                    || content_length_ > kMaxContentLength) {
                return Status::kInvalidContentLength;
            }
        }

        assert (content_length_ > 0);
        http_message_.AppendBody(data);
        auto current_body_size = http_message_.Body().size();
        const auto size = static_cast<size_t>(content_length_);
        if (current_body_size > size) {
            return Status::kTooMuchContent;
        } else if (current_body_size == size) {
            return Status::kDone;
        } else {
            DLOG_INFO << "Expected size = " << size
                << ", current size = " << current_body_size;
            return Status::kNeedsMore;
        }
    }
}
