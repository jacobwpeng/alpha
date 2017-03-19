/*
 * ==============================================================================
 *
 *       Filename:  SimpleHTTPServer.h
 *        Created:  05/03/15 19:11:14
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  Simple HTTP/1.0 server implementation
 *
 * ==============================================================================
 */

#pragma once

#include <map>
#include <alpha/Slice.h>
#include <alpha/NetAddress.h>
#include <alpha/TcpConnection.h>
#include <alpha/HTTPMessageCodec.h>

namespace alpha {
class EventLoop;
class TcpServer;
class NetAddress;
class HTTPMessage;
class SimpleHTTPServer {
 public:
  using HTTPHeader = HTTPMessageCodec::HTTPHeader;
  using Callback =
      std::function<void(TcpConnectionPtr, const HTTPMessage& message)>;
  using RequestCallback = std::function<void(
      TcpConnectionPtr, Slice path, const HTTPHeader& header, Slice data)>;
  SimpleHTTPServer(EventLoop* loop);
  ~SimpleHTTPServer();
  bool Run(const NetAddress& addr);
  void SetCallback(const Callback& cb) { callback_ = cb; }

 private:
  void DefaultRequestCallback(TcpConnectionPtr,
                              Slice method,
                              Slice path,
                              const HTTPHeader& header,
                              Slice data);
  void OnMessage(TcpConnectionPtr conn, TcpConnectionBuffer* buffer);
  void OnConnected(TcpConnectionPtr conn);
  void OnClose(TcpConnectionPtr conn);

  static const int kReadHeaderTimeout = 3000;  // ms
  EventLoop* loop_;
  std::unique_ptr<TcpServer> server_;
  Callback callback_;
};
}
