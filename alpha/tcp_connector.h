/*
 * =============================================================================
 *
 *       Filename:  tcp_connector.h
 *        Created:  04/06/15 15:18:03
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __TCP_CONNECTOR_H__
#define __TCP_CONNECTOR_H__

#include <map>
#include <memory>
#include <functional>
#include "compiler.h"
#include "channel.h"

namespace alpha {
class Channel;
class EventLoop;
class NetAddress;
class TcpConnector {
 public:
  using ConnectedCallback = std::function<void(int)>;
  using ErrorCallback = std::function<void(const NetAddress&)>;

  TcpConnector(EventLoop* loop);
  ~TcpConnector();
  DISABLE_COPY_ASSIGNMENT(TcpConnector);

  void SetOnConnected(const ConnectedCallback& cb) { connected_callback_ = cb; }
  void SetOnError(const ErrorCallback& cb) { error_callback_ = cb; }

  void ConnectTo(const NetAddress& addr);

 private:
  void OnConnected(int fd);
  void OnError(int fd, const NetAddress& addr);
  bool RemoveConnectingFd(int fd);
  bool AddConnectingFd(int fd, const NetAddress& addr);
  EventLoop* loop_;
  ConnectedCallback connected_callback_;
  ErrorCallback error_callback_;
  std::map<int, std::unique_ptr<Channel>> connecting_fds_;
};
}

#endif /* ----- #ifndef __TCP_CONNECTOR_H__  ----- */
