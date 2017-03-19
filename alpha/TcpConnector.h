/*
 * =============================================================================
 *
 *       Filename:  TcpConnector.h
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
#include <alpha/Compiler.h>
#include <alpha/ScopedGeneric.h>
#include <alpha/TimerManager.h>

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
  struct ConnectingFDInfo {
    alpha::ScopedFD fd;
    std::unique_ptr<Channel> channel;
    alpha::TimerManager::TimerId timeout_timer_id;
  };
  void OnConnected(int fd, const NetAddress& addr);
  void OnError(int fd, const NetAddress& addr);
  void OnConnectTimeout(int fd, const NetAddress& addr);
  bool RemoveConnectingFd(int fd);
  ConnectingFDInfo CheckRemoveConnectingFD(int fd, bool timeout = false);
  void AddConnectingFd(alpha::ScopedFD fd, const NetAddress& addr);
  void DeferDestroyChannel(std::unique_ptr<Channel>&& channel);
  EventLoop* loop_;
  ConnectedCallback connected_callback_;
  ErrorCallback error_callback_;
  // std::map<int, std::unique_ptr<Channel>> connecting_fds_;
  std::map<int, ConnectingFDInfo> connecting_fds_;
};
}

#endif /* ----- #ifndef __TCP_CONNECTOR_H__  ----- */
