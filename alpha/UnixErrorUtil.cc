/*
 * =============================================================================
 *
 *       Filename:  UnixErrorUtil.cc
 *        Created:  01/17/16 19:53:37
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <alpha/UnixErrorUtil.h>
#include <cstring>
#include <alpha/Compiler.h>
#include <alpha/Logger.h>

namespace alpha {
std::string UnixErrorToString(int errnum) {
  char buf[128];
  char* msg = ::strerror_r(errnum, buf, sizeof(buf));
  return msg;
}
}
