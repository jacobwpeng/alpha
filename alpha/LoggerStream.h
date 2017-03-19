/*
 * ==============================================================================
 *
 *       Filename:  LoggerStream.h
 *        Created:  12/21/14 05:42:38
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#ifndef __LOGGER_STREAM_H__
#define __LOGGER_STREAM_H__

#include <ostream>
#include <alpha/LoggerStreambuf.h>

namespace alpha {
class LoggerStream : public std::ostream {
 public:
  LoggerStream();
  virtual ~LoggerStream() = default;
  LoggerStreambuf* streambuf();

 private:
  LoggerStreambuf streambuf_;
};

class NullStream final : public LoggerStream {
 public:
  NullStream();
  virtual ~NullStream() = default;

 private:
  char message_buffer_[2];
};
}

namespace std {
template <typename T>
std::ostream& operator<<(
    typename std::enable_if<std::is_enum<T>::value, std::ostream>::type& stream,
    const T& e) {
  return stream << static_cast<typename std::underlying_type<T>::type>(e);
}
}

#endif /* ----- #ifndef __LOGGER_STREAM_H__  ----- */
