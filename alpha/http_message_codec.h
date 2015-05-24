/*
 * ==============================================================================
 *
 *       Filename:  http_message_codec.h
 *        Created:  05/03/15 19:40:11
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#ifndef  __HTTP_MESSAGE_CODEC_H__
#define  __HTTP_MESSAGE_CODEC_H__

#include <map>
#include <string>
#include <alpha/slice.h>
#include <alpha/tcp_connection.h>
#include <alpha/http_message.h>

namespace alpha {
    class HTTPMessageCodec {
        public:
            enum class Status {
                kDone = 0,
                kNeedsMore = 1,
                kUnknownError = 2,

                kInvalidMethod = 10,
                kInvalidStartLine = 11,
                kInvalidHTTPVersion = 12,
                kInvalidHeadLine = 13,
                kDuplicatedHead = 14,
                kInvalidContentLength = 15,
                kTooMuchContent = 16,

                kParseStartLine = 100,
                kParseHeader = 101,
                kParseEmptyLine = 102,
                kParseData = 103,
            };

            using HTTPHeader = std::map<std::string, std::string>;

            Status Process(Slice slice, int* consumed);
            HTTPMessage& Done();

        private:
            Status ParseStartLine(Slice start_line);
            Status ParseHeader(Slice data, int* consumed);
            Status OperationAfterParseHeader();
            Status AppendData(Slice data);
            Status status_ = Status::kParseStartLine;
            int content_length_ = -1;
            HTTPMessage http_message_;
    };
}

#endif   /* ----- #ifndef __HTTP_MESSAGE_CODEC_H__  ----- */
