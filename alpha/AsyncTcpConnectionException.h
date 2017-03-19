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

#pragma once

#include <stdexcept>

namespace alpha {
class AsyncTcpConnectionException : public std::runtime_error {
 public:
  explicit AsyncTcpConnectionException(const char* what);
};

class AsyncTcpConnectionOperationTimeout : public AsyncTcpConnectionException {
 public:
  explicit AsyncTcpConnectionOperationTimeout();
};

class AsyncTcpConnectionClosed : public AsyncTcpConnectionException {
 public:
  explicit AsyncTcpConnectionClosed(const char* what);
};
}

