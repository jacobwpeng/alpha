/*
 * =============================================================================
 *
 *       Filename:  AsyncTcpConnectionException.cc
 *        Created:  12/02/15 14:39:09
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "AsyncTcpConnectionException.h"

namespace alpha {
AsyncTcpConnectionException::AsyncTcpConnectionException(const char* what)
    : std::runtime_error(what) {}

AsyncTcpConnectionOperationTimeout::AsyncTcpConnectionOperationTimeout()
    : AsyncTcpConnectionException("timeout") {}

AsyncTcpConnectionClosed::AsyncTcpConnectionClosed(const char* what)
    : AsyncTcpConnectionException(
          std::string("Operate on closed connection: ").append(what).c_str()) {}
}
