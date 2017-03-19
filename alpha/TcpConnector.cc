/*
 * =============================================================================
 *
 *       Filename:  TcpConnector.cc
 *        Created:  04/06/15 15:31:00
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <alpha/TcpConnector.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <cassert>
#include <sstream>

#include <alpha/Compiler.h>
#include <alpha/Logger.h>
#include <alpha/Channel.h>
#include <alpha/EventLoop.h>
#include <alpha/NetAddress.h>
#include <alpha/SocketOps.h>
#include <alpha/UnixErrorUtil.h>
#include <alpha/ScopedGeneric.h>

namespace alpha {
TcpConnector::TcpConnector(EventLoop* loop) : loop_(loop) {}

TcpConnector::~TcpConnector() = default;

void TcpConnector::ConnectTo(const alpha::NetAddress& addr) {
  assert(connected_callback_);
  alpha::ScopedFD scoped_fd(::socket(AF_INET, SOCK_STREAM, 0));
  PCHECK(scoped_fd.is_valid()) << "Create socket failed, addr = " << addr;
  SocketOps::SetNonBlocking(scoped_fd.get());
  struct sockaddr_in sock_addr = addr.ToSockAddr();
  int err = ::connect(scoped_fd.get(),
                      reinterpret_cast<sockaddr*>(&sock_addr),
                      sizeof(sock_addr));
  if (err) {
    if (errno == EINPROGRESS) {
      DLOG_INFO << "connect inprogress to " << addr;
      AddConnectingFd(std::move(scoped_fd), addr);
    } else {
      PLOG_WARNING << "connect to " << addr << " failed immediately";
      if (error_callback_) {
        error_callback_(addr);
      }
    }
  } else {
    DLOG_INFO << "connected to " << addr
              << " immediately, fd: " << scoped_fd.get();
    connected_callback_(scoped_fd.Release());
  }
}

void TcpConnector::OnConnected(int fd, const NetAddress& addr) {
  auto connecting_fd_info = CheckRemoveConnectingFD(fd);
  DeferDestroyChannel(std::move(connecting_fd_info.channel));
  int err = SocketOps::GetAndClearError(fd);
  if (err) {
    DLOG_INFO << "Writable when connect error";
    if (error_callback_) error_callback_(addr);
  } else {
    DLOG_INFO << "connected to " << addr << ", fd: " << fd;
    connected_callback_(connecting_fd_info.fd.Release());
  }
}

void TcpConnector::OnError(int fd, const alpha::NetAddress& addr) {
  auto connecting_fd_info = CheckRemoveConnectingFD(fd);
  connecting_fd_info.channel->set_write_callback(nullptr);
  DeferDestroyChannel(std::move(connecting_fd_info.channel));
  int err = SocketOps::GetAndClearError(fd);
  auto msg = UnixErrorToString(err);
  if (err == ECONNREFUSED) {
    // 不把这个当做是异常
    LOG_INFO << msg << ", addr = " << addr << ", fd: " << fd;
  } else {
    LOG_WARNING << msg << ", addr = " << addr << ", fd: " << fd;
  }
  if (error_callback_) {
    error_callback_(addr);
  }
}

void TcpConnector::OnConnectTimeout(int fd, const NetAddress& addr) {
  auto connecting_fd_info = CheckRemoveConnectingFD(fd, true);
  LOG_INFO << "Connecting to " << addr << " timeout, fd: " << fd;
  (void)connecting_fd_info;
  if (error_callback_) {
    error_callback_(addr);
  }
}

TcpConnector::ConnectingFDInfo TcpConnector::CheckRemoveConnectingFD(
    int fd, bool timeout) {
  auto it = connecting_fds_.find(fd);
  CHECK(it != connecting_fds_.end()) << "fd not found, fd: " << fd;
  auto info = std::move(it->second);
  connecting_fds_.erase(it);
  if (!timeout) {
    loop_->RemoveTimer(info.timeout_timer_id);
    info.timeout_timer_id = 0;
  }
  return std::move(info);
}

void TcpConnector::AddConnectingFd(alpha::ScopedFD fd, const NetAddress& addr) {
  int raw_fd = fd.get();
  LOG_INFO << "Add connecting fd: " << raw_fd;
  using namespace std::placeholders;
  auto it = connecting_fds_.find(raw_fd);
  CHECK(it == connecting_fds_.end()) << "Same fd in connecting map, fd: "
                                     << raw_fd;
  ConnectingFDInfo info;
  info.channel = alpha::make_unique<Channel>(loop_, raw_fd);
  info.channel->set_error_callback(
      std::bind(&TcpConnector::OnError, this, raw_fd, addr));
  info.channel->set_write_callback(
      std::bind(&TcpConnector::OnConnected, this, raw_fd, addr));
  info.channel->EnableWriting();
  static const int kDefaultConnectTimeout = 5000;  // ms
  info.timeout_timer_id = loop_->RunAfter(
      kDefaultConnectTimeout,
      std::bind(&TcpConnector::OnConnectTimeout, this, raw_fd, addr));
  info.fd = std::move(fd);
  connecting_fds_.emplace(raw_fd, std::move(info));
}

void TcpConnector::DeferDestroyChannel(std::unique_ptr<Channel>&& channel) {
  channel->DisableAll();
  channel->Remove();
  std::shared_ptr<Channel> shared_channel(std::move(channel));
  loop_->QueueInLoop([shared_channel] {});
}
}
