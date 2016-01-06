/*
 * =============================================================================
 *
 *       Filename:  AsyncTcpConnection.h
 *        Created:  12/01/15 10:39:41
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include "slice.h"
#include "tcp_connection.h"
#include "AsyncTcpConnectionCoroutine.h"
#include "IOBuffer.h"

namespace alpha {
class AsyncTcpConnection {
 public:
  enum class Status : uint8_t {
    kIdle = 0,
    kClosed = 1,
    kWaitingMessage = 2,
    kWaitingWriteDone = 3,
    kWaitingClose = 4
  };
  static const int kNoTimeout = -1;

  AsyncTcpConnection(TcpConnectionPtr& conn, AsyncTcpConnectionCoroutine* co);
  void Write(const void* data, size_t size);
  void Write(alpha::Slice data);
  size_t Read(IOBuffer* buf, size_t buf_len, size_t bytes = 0);
  std::string Read(size_t bytes = 0, int timeout = kNoTimeout);
  std::string ReadCached(size_t bytes = 0);
  alpha::Slice PeekCached() const;
  void Close();
  // Coroutine* co() { return co_; }
  bool HasCachedData() const;
  size_t CachedDataSize() const;
  Status status() const {
    return status_;
  };
  bool closed() const {
    return status_ == Status::kClosed;
  };

 private:
  friend class AsyncTcpClient;
  void set_status(Status status) { status_ = status; }
  void SetWaitingWriteDone() { status_ = Status::kWaitingWriteDone; }
  void SetIdle() { status_ = Status::kIdle; }
  void SetWaitingMessage() { status_ = Status::kWaitingMessage; }
  void set_tcp_connection(TcpConnectionPtr& conn) { conn_ = conn; }

  Status status_;
  TcpConnectionPtr conn_;
  AsyncTcpConnectionCoroutine* co_;
};
}
