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

#ifndef __ASYNCTCPCONNECTION_H__
#define __ASYNCTCPCONNECTION_H__

#include "slice.h"
#include "coroutine.h"
#include "tcp_connection.h"

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

  AsyncTcpConnection(TcpConnectionPtr& conn, Coroutine* co);
  void Write(alpha::Slice data);
  // size_t Read(char* out, size_t bytes = 0);
  std::string Read(size_t bytes = 0);
  std::string ReadCached(size_t bytes = 0);
  void Close();
  // Coroutine* co() { return co_; }
  Status status() const { return status_; };
  bool closed() const { return status_ == Status::kClosed; };

 private:
  friend class AsyncTcpClient;
  void set_status(Status status) { status_ = status; }
  void set_tcp_connection(TcpConnectionPtr& conn) { conn_ = conn; }

  Status status_;
  TcpConnectionPtr conn_;
  Coroutine* co_;
};
}

#endif /* ----- #ifndef __ASYNCTCPCONNECTION_H__  ----- */
