/*
 * =============================================================================
 *
 *       Filename:  tcp_client.cc
 *        Created:  04/06/15 15:52:59
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "tcp_client.h"

#include <cassert>

#include "logger.h"
#include "event_loop.h"
#include "net_address.h"
#include "tcp_connector.h"

namespace alpha {
TcpClient::TcpClient(EventLoop* loop) : loop_(loop) {
  assert(loop);
  connector_.reset(new TcpConnector(loop));

  using namespace std::placeholders;
  connector_->SetOnConnected(std::bind(&TcpClient::OnConnected, this, _1));
  connector_->SetOnError(std::bind(&TcpClient::OnConnectError, this, _1));
}

TcpClient::~TcpClient() {
  for (auto& p : connections_) {
    ::close(p.first);
  }

  for (const auto& p : auto_reconnect_timer_ids_) {
    loop_->RemoveTimer(p.second);
  }
}

void TcpClient::ConnectTo(const NetAddress& addr, bool auto_reconnect) {
  loop_->QueueInLoop(
      std::bind(&TcpConnector::ConnectTo, connector_.get(), addr));
  if (auto_reconnect) {
    auto_reconnect_addresses_.insert(addr);
  } else {
    RemoveAutoReconnect(addr);
  }
}

void TcpClient::OnConnected(int fd) {
  assert(connections_.find(fd) == connections_.end());
  TcpConnection::State state = TcpConnection::State::kConnected;
  TcpConnectionPtr conn = std::make_shared<TcpConnection>(loop_, fd, state);

  using namespace std::placeholders;
  conn->SetOnClose(std::bind(&TcpClient::OnClose, this, _1));
  connected_callback_(conn);
  connections_.emplace(fd, conn);
}

void TcpClient::OnClose(int fd) {
  auto it = connections_.find(fd);
  assert(it != connections_.end());
  TcpConnectionPtr conn = it->second;
  connections_.erase(it);

  DLOG_INFO << "Connection to " << it->second->PeerAddr() << " is closed";
  if (close_callback_) {
    close_callback_(conn);
  }
  MaybeReconnect(conn->PeerAddr());
}

void TcpClient::OnConnectError(const NetAddress& addr) {
  if (connect_error_callback_) {
    connect_error_callback_(addr);
  }
  DLOG_INFO << "MaybeReconnect, addr = " << addr;
  MaybeReconnect(addr);
}

void TcpClient::MaybeReconnect(const NetAddress& addr) {
  if (auto_reconnect_addresses_.find(addr) != auto_reconnect_addresses_.end()) {
    DLOG_INFO << "Connect to " << addr << " failed, try to reconnect";
    auto timer_id =
        loop_->RunAfter(reconnect_interval_,
                        std::bind(&TcpClient::ConnectTo, this, addr, true));
    auto it = auto_reconnect_timer_ids_.find(addr);
    if (it == auto_reconnect_timer_ids_.end()) {
      auto_reconnect_timer_ids_.insert(std::make_pair(addr, timer_id));
    } else {
      it->second = timer_id;
    }
  } else {
    DLOG_INFO << "Giveup reconnect to addr";
  }
}

void TcpClient::RemoveAutoReconnect(const NetAddress& addr) {
  auto it = auto_reconnect_addresses_.find(addr);
  if (it != auto_reconnect_addresses_.end()) {
    DLOG_INFO << "RemoveAutoReconnect, addr = " << addr;
    auto_reconnect_addresses_.erase(it);
  }

  auto timer_iter = auto_reconnect_timer_ids_.find(addr);
  if (timer_iter != auto_reconnect_timer_ids_.end()) {
    loop_->RemoveTimer(timer_iter->second);
  }
}
}
