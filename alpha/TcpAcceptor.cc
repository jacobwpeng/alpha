/*
 * =============================================================================
 *
 *       Filename:  TcpAcceptor.cc
 *        Created:  04/05/15 15:12:31
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */
#include <alpha/TcpAcceptor.h>

#include <netinet/in.h>
#include <cassert>

#include <alpha/Compiler.h>
#include <alpha/Logger.h>
#include <alpha/Channel.h>
#include <alpha/NetAddress.h>
#include <alpha/EventLoop.h>
#include <alpha/SocketOps.h>
#include <alpha/ScopedGeneric.h>

namespace alpha {
TcpAcceptor::TcpAcceptor(EventLoop* loop) : loop_(loop), listen_fd_(-1) {}

TcpAcceptor::~TcpAcceptor() {
  if (listen_fd_ != -1) {
    ::close(listen_fd_);
  }
}

bool TcpAcceptor::Bind(const alpha::NetAddress& addr) {
  assert(listen_fd_ == -1);
  listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
  if (unlikely(listen_fd_ == -1)) {
    PLOG_ERROR << "create socket failed";
    return false;
  }
  SocketOps::SetReuseAddress(listen_fd_);
  struct sockaddr_in sock_addr = addr.ToSockAddr();
  int ret = ::bind(
      listen_fd_, reinterpret_cast<sockaddr*>(&sock_addr), sizeof(sockaddr));
  if (unlikely(ret == -1)) {
    PLOG_ERROR << "bind to " << addr.ip() << ":" << addr.port() << " failed";
    return false;
  }
  const int kTimeoutMicroSeconds = 5000000;
  SocketOps::SetReceiveTimeout(listen_fd_, kTimeoutMicroSeconds);
  SocketOps::SetNonBlocking(listen_fd_);
  return true;
}

bool TcpAcceptor::Listen() {
  CHECK(new_connection_callback_);
  int ret = ::listen(listen_fd_, SOMAXCONN);
  if (unlikely(ret == -1)) {
    PLOG_ERROR << "listen failed";
    return false;
  } else {
    channel_.reset(new Channel(loop_, listen_fd_));
    channel_->set_read_callback(std::bind(&TcpAcceptor::OnNewConnection, this));
    channel_->EnableReading();
    return true;
  }
}

void TcpAcceptor::OnNewConnection() {
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  alpha::ScopedFD fd(
      ::accept(listen_fd_, reinterpret_cast<sockaddr*>(&addr), &len));
  if (unlikely(!fd.is_valid())) {
    PLOG_WARNING << "accept failed";
  } else {
    int err = SocketOps::GetAndClearError(fd.get());
    if (unlikely(err)) {
      LOG_INFO << "Error immediately after accept, err: " << err;
    } else {
      SocketOps::SetNonBlocking(fd.get());
      DLOG_INFO << "New Connection, fd = " << fd.get();
      new_connection_callback_(fd.Release());
    }
  }
}
}
