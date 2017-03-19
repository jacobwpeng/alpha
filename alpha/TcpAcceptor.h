/*
 * =============================================================================
 *
 *       Filename:  TcpAcceptor.h
 *        Created:  04/05/15 15:08:16
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __TCP_ACCEPTOR_H__
#define __TCP_ACCEPTOR_H__

#include <memory>
#include <functional>
#include <alpha/TcpConnection.h>

namespace alpha {
class Channel;
class NetAddress;
class EventLoop;

class TcpAcceptor {
 public:
  using NewConnectionCallback = std::function<void(int)>;

  TcpAcceptor(EventLoop* loop);
  ~TcpAcceptor();
  DISABLE_COPY_ASSIGNMENT(TcpAcceptor);

  void SetOnNewConnection(const NewConnectionCallback& cb) {
    new_connection_callback_ = cb;
  }

  bool Bind(const NetAddress& addr);
  bool Listen();

 private:
  void OnNewConnection();

  EventLoop* loop_;
  int listen_fd_;
  std::unique_ptr<Channel> channel_;
  NewConnectionCallback new_connection_callback_;
};
}

#endif /* ----- #ifndef __TCP_ACCEPTOR_H__  ----- */
