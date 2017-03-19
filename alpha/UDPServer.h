/*
 * =============================================================================
 *
 *       Filename:  UDPServer.h
 *        Created:  12/29/15 14:58:14
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include <memory>
#include <functional>
#include <alpha/Compiler.h>
#include <alpha/NetAddress.h>
#include <alpha/UDPSocket.h>

namespace alpha {
class Channel;
class EventLoop;

class UDPServer final {
 public:
  using MessageCallback = std::function<void(
      UDPSocket*, IOBuffer*, size_t buf_len, const NetAddress&)>;

  explicit UDPServer(alpha::EventLoop* loop);
  ~UDPServer();
  DISABLE_COPY_ASSIGNMENT(UDPServer);

  void SetMessageCallback(const MessageCallback& cb) { cb_ = cb; }
  bool Run(const NetAddress& address);

 private:
  void OnReadable();
  EventLoop* loop_;
  MessageCallback cb_;
  std::unique_ptr<Channel> channel_;
  UDPSocket socket_;
};
}
