/*
 * ==============================================================================
 *
 *       Filename:  LoggerStreambuf.h
 *        Created:  12/21/14 05:37:41
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#ifndef __LOGGER_STREAMBUF_H__
#define __LOGGER_STREAMBUF_H__

#include <streambuf>

namespace alpha {
class LoggerStreambuf : public std::streambuf {
 public:
  int used();

  int overflow(int c);

 private:
  std::streambuf* setbuf(char* s, std::streamsize n);

 private:
  char tmp_;
};
}

#endif /* ----- #ifndef __LOGGER_STREAMBUF_H__  ----- */
