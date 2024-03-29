/*
 * =============================================================================
 *
 *       Filename:  AsyncTcpClient.cc
 *        Created:  12/01/15 11:08:45
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <alpha/AsyncTcpClient.h>
#include <alpha/EventLoop.h>
#include <alpha/Logger.h>
#include <algorithm>

namespace alpha {
using namespace std::placeholders;
AsyncTcpClient::AsyncTcpClient(EventLoop* loop)
    : loop_(loop), tcp_client_(loop) {
  tcp_client_.SetOnConnected(std::bind(&AsyncTcpClient::OnConnected, this, _1));
  tcp_client_.SetOnConnectError(
      std::bind(&AsyncTcpClient::OnConnectError, this, _1));
  tcp_client_.SetOnClose(std::bind(&AsyncTcpClient::OnClosed, this, _1));
}

void AsyncTcpClient::RunInCoroutine(
    const AsyncTcpConnectionCoroutine::CoroutineFunc& func) {
  auto co = alpha::make_unique<AsyncTcpConnectionCoroutine>(this, func);
  coroutines_.emplace_back(std::move(co));
  (*coroutines_.rbegin())->Resume();
}

std::shared_ptr<AsyncTcpConnection> AsyncTcpClient::ConnectTo(
    const NetAddress& addr, AsyncTcpConnectionCoroutine* co) {
  tcp_client_.ConnectTo(addr);
  ConnectionInfo info;
  info.co = co;
  MapConnecting(addr, info);
  co->Yield();
  auto it = FindConnecting(addr);
  auto conn = it->second.async_conn;
  RemoveConnecting(it);
  return conn;
}

void AsyncTcpClient::OnConnected(TcpConnectionPtr conn) {
  conn->SetOnRead(std::bind(&AsyncTcpClient::OnMessage, this, _1, _2));
  conn->SetOnWriteDone(std::bind(&AsyncTcpClient::OnWriteDone, this, _1));
  auto it = FindConnecting(conn->PeerAddr());
  auto info = &it->second;
  auto co = info->co;
  info->async_conn = std::make_shared<AsyncTcpConnection>(conn, co);
  MapConnected(conn.get(), *info);
  DLOG_INFO << "Connected resume, id: " << co->id()
            << ", addr: " << conn->PeerAddr();
  co->Resume();
}

void AsyncTcpClient::OnConnectError(const NetAddress& addr) {
  auto it = FindConnecting(addr);
  auto info = &it->second;
  auto co = info->co;
  DLOG_INFO << "ConnectError resume, id: " << co->id() << ", addr: " << addr;
  co->Resume();
}

void AsyncTcpClient::OnClosed(TcpConnectionPtr conn) {
  auto info = FindConnected(conn.get());
  CHECK(info->async_conn);
  auto old_connection_status = info->async_conn->status();
  info->async_conn->set_status(AsyncTcpConnection::Status::kClosed);
  CHECK(info->co->status() <= Coroutine::Status::kDead)
      << "Invalid status: " << info->co->status() << ", info->co: " << info->co;
  if (info->co->IsSuspended() &&
      old_connection_status != AsyncTcpConnection::Status::kIdle) {
    DLOG_INFO << "Closed resume, id: " << info->co->id()
              << ", addr: " << conn->PeerAddr();
    info->co->Resume();
  }
  RemoveConnected(conn.get());
}

void AsyncTcpClient::OnMessage(TcpConnectionPtr conn, TcpConnectionBuffer*) {
  auto info = FindConnected(conn.get());
  CHECK(info->async_conn);
  if (info->async_conn->status() ==
      AsyncTcpConnection::Status::kWaitingMessage) {
    DLOG_INFO << "Message resume, id: " << info->co->id()
              << ", addr: " << conn->PeerAddr();
    info->co->Resume();
  }
}

void AsyncTcpClient::OnWriteDone(TcpConnectionPtr conn) {
  auto info = FindConnected(conn.get());
  CHECK(info->async_conn);
  if (info->async_conn->status() ==
      AsyncTcpConnection::Status::kWaitingWriteDone) {
    DLOG_INFO << "WriteDone resume, id: " << info->co->id()
              << ", addr: " << conn->PeerAddr();
    info->co->Resume();
  }
}

void AsyncTcpClient::MapConnecting(const NetAddress& addr,
                                   const ConnectionInfo& info) {
  connecting_.push_back(std::make_pair(addr, info));
}

AsyncTcpClient::ConnectingArray::iterator AsyncTcpClient::FindConnecting(
    const NetAddress& addr) {
  // Always find the first pair that matches this addr to simulate FIFO
  auto it = std::find_if(connecting_.begin(),
                         connecting_.end(),
                         [&addr](const ConnectingArray::value_type& v) {
                           return v.first == addr;
                         });
  CHECK(it != connecting_.end());
  return it;
}

void AsyncTcpClient::RemoveConnecting(ConnectingArray::iterator it) {
  CHECK(!connecting_.empty());
  CHECK(it != connecting_.end());
  connecting_.erase(it);
}

void AsyncTcpClient::MapConnected(TcpConnection* conn,
                                  const ConnectionInfo& info) {
  auto p = connected_.emplace(conn, info);
  CHECK(p.second);
  (void)p;
}

AsyncTcpClient::ConnectionInfo* AsyncTcpClient::FindConnected(
    TcpConnection* conn) {
  auto it = connected_.find(conn);
  CHECK(it != connected_.end());
  return &it->second;
}

void AsyncTcpClient::RemoveConnected(TcpConnection* conn) {
  auto num = connected_.erase(conn);
  CHECK(num == 1);
}

void AsyncTcpClient::RemoveCoroutine(AsyncTcpConnectionCoroutine* co) {
  CHECK(!coroutines_.empty());
  auto it = std::find_if(
      coroutines_.begin(),
      coroutines_.end(),
      [co](std::unique_ptr<AsyncTcpConnectionCoroutine>& coroutine) {
        return coroutine.get() == co;
      });
  CHECK(it != coroutines_.end());
  coroutines_.erase(it);
}
}  // namespace alpha
