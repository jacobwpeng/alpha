/*
 * =============================================================================
 *
 *       Filename:  UDPSocket.cc
 *        Created:  12/29/15 10:31:54
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "UDPSocket.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <alpha/logger.h>
#include <alpha/socket_ops.h>

namespace alpha {
UDPSocket::UDPSocket() = default;

UDPSocket::~UDPSocket() { Close(); }

int UDPSocket::Open() {
  socket_ = ::socket(AF_INET, SOCK_DGRAM, 0);
  if (socket_ == kInvalidSocket) {
    return -1;
  }
  SocketOps::SetNonBlocking(socket_);
  return 0;
}

int UDPSocket::Connect(const NetAddress& addr) {
  CHECK(socket_ != kInvalidSocket);
  sockaddr_in sock_addr = addr.ToSockAddr();
  socklen_t addr_len = sizeof(sock_addr);
  int rc = ::connect(socket_, reinterpret_cast<struct sockaddr*>(&sock_addr),
                     addr_len);
  if (rc == -1) {
    return -1;
  } else {
    is_connected_ = true;
    return 0;
  }
}

int UDPSocket::Bind(const NetAddress& addr) {
  CHECK(socket_ != kInvalidSocket);
  CHECK(!is_connected());
  int rc = DoBind(addr);
  if (rc == -1) {
    return -1;
  } else {
    is_connected_ = true;
    return 0;
  }
}

void UDPSocket::Close() {
  if (socket_ == kInvalidSocket) {
    return;
  }
  PCHECK(::close(socket_) == 0);
  socket_ = kInvalidSocket;
  is_connected_ = false;
}

int UDPSocket::GetPeerAddress(NetAddress* addr) {
  if (is_connected()) return -1;
  struct sockaddr_storage storage;
  struct sockaddr* const sock_addr =
      reinterpret_cast<struct sockaddr*>(&storage);
  socklen_t addr_len = sizeof(storage);
  if (getpeername(socket_, sock_addr, &addr_len)) {
    return -1;
  }
  CHECK(addr_len == sizeof(sockaddr_in));
  if (!addr->FromSockAddr(*reinterpret_cast<struct sockaddr_in*>(sock_addr))) {
    return -2;
  } else {
    return 0;
  }
}

int UDPSocket::GetLocalAddress(NetAddress* addr) {
  if (!is_connected()) return -1;
  struct sockaddr_storage storage;
  struct sockaddr* const sock_addr =
      reinterpret_cast<struct sockaddr*>(&storage);
  socklen_t addr_len = sizeof(storage);
  if (getsockname(socket_, sock_addr, &addr_len)) {
    PLOG_INFO << "getsockname";
    return -1;
  }
  CHECK(addr_len == sizeof(sockaddr_in));
  if (!addr->FromSockAddr(*reinterpret_cast<struct sockaddr_in*>(sock_addr))) {
    return -2;
  } else {
    return 0;
  }
}

int UDPSocket::Read(IOBuffer* buf, size_t buf_len) {
  return RecvFrom(buf, buf_len, nullptr);
}

int UDPSocket::Write(IOBuffer* buf, size_t buf_len) {
  return SendOrWrite(buf, buf_len, nullptr);
}

int UDPSocket::RecvFrom(IOBuffer* buf, size_t buf_len, NetAddress* addr) {
  CHECK(socket_ != kInvalidSocket);
  struct sockaddr_storage storage;
  struct sockaddr* const sock_addr =
      reinterpret_cast<struct sockaddr*>(&storage);
  socklen_t addr_len = sizeof(storage);
  int nread =
      ::recvfrom(socket_, buf->data(), buf_len, 0, sock_addr, &addr_len);
  if (nread >= 0) {
    if (addr) {
      CHECK(sizeof(sockaddr_in) == addr_len);
      CHECK(addr->FromSockAddr(*reinterpret_cast<sockaddr_in*>(sock_addr)));
    }
    return nread;
  } else {
    return -1;
  }
}

int UDPSocket::SendTo(IOBuffer* buf, size_t buf_len, const NetAddress& addr) {
  return SendOrWrite(buf, buf_len, &addr);
}

int UDPSocket::DoBind(const NetAddress& addr) {
  sockaddr_in sock_addr = addr.ToSockAddr();
  socklen_t addr_len = sizeof(sock_addr);
  return ::bind(socket_, reinterpret_cast<struct sockaddr*>(&sock_addr),
                addr_len);
}

int UDPSocket::SendOrWrite(IOBuffer* buf, size_t buf_len,
                           const NetAddress* addr) {
  CHECK(socket_ != kInvalidSocket);
  if (!addr) {
    return ::sendto(socket_, buf->data(), buf_len, 0, nullptr, 0);
  } else {
    sockaddr_in sock_addr = addr->ToSockAddr();
    return ::sendto(socket_, buf->data(), buf_len, 0,
                    reinterpret_cast<struct sockaddr*>(&sock_addr),
                    sizeof(sock_addr));
  }
}

int UDPSocket::SetReceiveBufferSize(int32_t size) {
  return setsockopt(socket_, SOL_SOCKET, SO_SNDBUF,
                    reinterpret_cast<const char*>(&size), sizeof(size));
}

int UDPSocket::SetSendBufferSize(int32_t size) {
  return setsockopt(socket_, SOL_SOCKET, SO_RCVBUF,
                    reinterpret_cast<const char*>(&size), sizeof(size));
}
}
