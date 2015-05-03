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
    HTTPMessageCodec::Status HTTPMessageCodec::Process(Slice slice, int* consumed) {
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

    HTTPMessageCodec::Status HTTPMessageCodec::ParseStartLine(Slice start_line) {
            const auto kVersionLength = Slice(" HTTP/1.0").size();
            if (!start_line.EndsWith(" HTTP/1.0")
                    && !start_line.EndsWith(" HTTP/1.1")) {
                return Status::kInvalidHTTPVersion;
            }
            size_t method_size = 0;
            if (start_line.StartsWith("GET /")) {
                method_ = Method::kGet;
                method_size = 4;
            } else if (start_line.StartsWith("PUT /")) {
                method_ = Method::kPut;
                method_size = 4;
            } else if (start_line.StartsWith("POST /")) {
                method_ = Method::kPost;
                method_size = 5;
            }
            else if (start_line.StartsWith("DELETE /")) {
                method_ = Method::kDelete;
                method_size = 7;
            } else {
                return Status::kInvalidMethod;
            }
            path_ = start_line.RemovePrefix(method_size).RemoveSuffix(kVersionLength).ToString();
        return Status::kParseHeader;
    }

    HTTPMessageCodec::Status HTTPMessageCodec::ParseHeader(Slice data, int* consumed) {
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
            if (headers_.find(key.ToString()) != headers_.end()) {
                return Status::kDuplicatedHead;
            }

            headers_.emplace(key.ToString(), val.ToString());
            *consumed += line.size() + CRLF.size();
            data = data.RemovePrefix(line.size() + CRLF.size());
            pos = data.find(CRLF);
        }
        return Status::kNeedsMore;
    }

    HTTPMessageCodec::Status HTTPMessageCodec::OperationAfterParseHeader() {
        if (headers_.find("Content-Length") == headers_.end()) {
            return Status::kDone;
        } else {
            return Status::kParseData;
        }
    }

    HTTPMessageCodec::Status HTTPMessageCodec::AppendData(Slice data) {
        if (content_length_ == -1) {
            const size_t kMaxContentLengthDigitNum = 12;
            const int kMaxContentLength = 1024;
            auto it = headers_.find("Content-Length");
            assert (it != headers_.end());
            if (it->second.size() >= kMaxContentLengthDigitNum) {
                return Status::kInvalidContentLength;
            }

            auto n = ::sscanf(it->second.data(), "%d", &content_length_);
            if (n != 1 || content_length_ <= 0 || content_length_ > kMaxContentLength) {
                return Status::kInvalidContentLength;
            }
        }

        assert (content_length_ > 0);
        data_.append(data.data(), data.size());
        const auto size = static_cast<size_t>(content_length_);
        if (data_.size() > size) {
            LOG_WARNING << "data_.size() = " << data_.size()
                << ", size = " << size;
            return Status::kTooMuchContent;
        } else if (data_.size() == size) {
            return Status::kDone;
        } else {
            DLOG_INFO << "Expected size = " << size
                << ", current size = " << data_.size();
            return Status::kNeedsMore;
        }
    }

    Slice HTTPMessageCodec::method_name() const {
        switch (method_) {
            case Method::kGet:
                return "GET";
            case Method::kPut:
                return "PUT";
            case Method::kPost:
                return "POST";
            case Method::kDelete:
                return "DELETE";
            default:
                return "UNKNOWN";
        }
    }
}
