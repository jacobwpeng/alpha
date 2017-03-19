/*
 * =============================================================================
 *
 *       Filename:  TcpClient.h
 *        Created:  04/06/15 15:42:58
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __TCP_CLIENT_H__
#define __TCP_CLIENT_H__

#include <map>
#include <set>
#include <alpha/Compiler.h>
#include <alpha/NetAddress.h>
#include <alpha/TcpConnection.h>
#include <alpha/TimerManager.h>

namespace alpha {
class Channel;
class EventLoop;
class NetAddress;
class TcpConnector;
class TcpClient {
 public:
  using ConnectedCallback = std::function<void(TcpConnectionPtr)>;
  using CloseCallback = std::function<void(TcpConnectionPtr)>;
  using ConnectErrorCallback = std::function<void(const NetAddress&)>;

  TcpClient(EventLoop* loop);
  ~TcpClient();
  DISABLE_COPY_ASSIGNMENT(TcpClient);

  void ConnectTo(const NetAddress& addr, bool auto_reconnect = false);
  void SetOnConnected(const ConnectedCallback& cb) { connected_callback_ = cb; }
  void SetOnClose(const CloseCallback& cb) { close_callback_ = cb; }
  void SetOnConnectError(const ConnectErrorCallback& cb) {
    connect_error_callback_ = cb;
  }

 private:
  void OnConnected(int fd);
  void OnClose(int fd);
  void OnConnectError(const NetAddress&);
  void MaybeReconnect(const NetAddress& addr);
  void RemoveAutoReconnect(const NetAddress& addr);

  static const int kDefaultReconnectInterval = 1000;  // ms
  using TcpConnectionMap = std::map<int, TcpConnectionPtr>;
  using ReconnectAddressSet = std::set<NetAddress>;
  using ReconnectTimerIdMap = std::map<NetAddress, TimerManager::TimerId>;
  int reconnect_interval_ = kDefaultReconnectInterval;
  EventLoop* loop_;
  TcpConnectionMap connections_;
  ReconnectAddressSet auto_reconnect_addresses_;
  ReconnectTimerIdMap auto_reconnect_timer_ids_;
  std::unique_ptr<TcpConnector> connector_;
  ConnectedCallback connected_callback_;
  CloseCallback close_callback_;
  ConnectErrorCallback connect_error_callback_;
};
}

#endif /* ----- #ifndef __TCP_CLIENT_H__  ----- */
