/*
 * ==============================================================================
 *
 *       Filename:  LoggerStream.cc
 *        Created:  12/21/14 05:43:31
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#include <alpha/LoggerStream.h>

namespace alpha {
LoggerStream::LoggerStream() { rdbuf(&streambuf_); }

LoggerStreambuf* LoggerStream::streambuf() { return &streambuf_; }

NullStream::NullStream() {
  LoggerStream::streambuf()->pubsetbuf(message_buffer_, 1);
}
}
