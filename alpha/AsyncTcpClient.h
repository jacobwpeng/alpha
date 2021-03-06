/*
 * =============================================================================
 *
 *       Filename:  AsyncTcpClient.h
 *        Created:  12/01/15 11:06:03
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include <vector>
#include <alpha/TcpClient.h>
#include <alpha/AsyncTcpConnection.h>
#include <alpha/AsyncTcpConnectionCoroutine.h>

namespace alpha {
class EventLoop;
class AsyncTcpClient {
 public:
  AsyncTcpClient(EventLoop* loop);
  void RunInCoroutine(const AsyncTcpConnectionCoroutine::CoroutineFunc& func);
  std::shared_ptr<AsyncTcpConnection> ConnectTo(
      const NetAddress& addr, AsyncTcpConnectionCoroutine* co);
  EventLoop* loop() const { return loop_; }

 private:
  struct ConnectionInfo {
    std::shared_ptr<AsyncTcpConnection> async_conn;
    AsyncTcpConnectionCoroutine* co;
  };
  using ConnectingArray = std::vector<std::pair<NetAddress, ConnectionInfo>>;
  using ConnectedMap = std::map<TcpConnection*, ConnectionInfo>;

  void OnConnected(TcpConnectionPtr conn);
  void OnConnectError(const NetAddress& addr);
  void OnClosed(TcpConnectionPtr conn);
  void OnMessage(TcpConnectionPtr conn, TcpConnectionBuffer*);
  void OnWriteDone(TcpConnectionPtr conn);

  void MapConnecting(const NetAddress& addr, const ConnectionInfo& info);
  ConnectingArray::iterator FindConnecting(const NetAddress& addr);
  void RemoveConnecting(ConnectingArray::iterator it);
  void MapConnected(TcpConnection* conn, const ConnectionInfo& info);
  ConnectionInfo* FindConnected(TcpConnection* conn);
  void RemoveConnected(TcpConnection* conn);
  void RemoveCoroutine(AsyncTcpConnectionCoroutine* co);
  EventLoop* loop_;
  TcpClient tcp_client_;
  std::vector<std::unique_ptr<AsyncTcpConnectionCoroutine>> coroutines_;
  ConnectingArray connecting_;
  ConnectedMap connected_;

  friend class AsyncTcpConnectionCoroutine;
};
}

