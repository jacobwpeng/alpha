/*
 * =============================================================================
 *
 *       Filename:  AsyncTcpConnectionException.h
 *        Created:  12/02/15 14:35:35
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __ASYNCTCPCONNECTIONEXCEPTION_H__
#define __ASYNCTCPCONNECTIONEXCEPTION_H__

#include <stdexcept>

namespace alpha {
class AsyncTcpConnectionException : public std::logic_error {
 public:
  explicit AsyncTcpConnectionException(const char* what);
};
}

#endif /* ----- #ifndef __ASYNCTCPCONNECTIONEXCEPTION_H__  ----- */
