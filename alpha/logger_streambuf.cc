/*
 * ==============================================================================
 *
 *       Filename:  logger_streambuf.cc
 *        Created:  12/21/14 05:38:34
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#include "logger_streambuf.h"

namespace alpha {
int LoggerStreambuf::used() { return pptr() - pbase(); }

int LoggerStreambuf::overflow(int c) { return epptr() == pptr() ? tmp_ : c; }

std::streambuf* LoggerStreambuf::setbuf(char* s, std::streamsize n) {
  setp(s, s + n);
  return this;
}
}
