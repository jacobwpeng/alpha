/*
 * =============================================================================
 *
 *       Filename:  NetAddress.cc
 *        Created:  04/05/15 11:28:16
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <alpha/NetAddress.h>

#include <cassert>
#include <cstring>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ostream>

#include <alpha/Compiler.h>
#include <alpha/Logger.h>

namespace alpha {
std::ostream& operator<<(std::ostream& os, const NetAddress& addr) {
  os << addr.FullAddress();
  return os;
}

bool operator==(const NetAddress& lhs, const NetAddress& rhs) {
  return lhs.ip() == rhs.ip() && lhs.port() == rhs.port();
}

bool operator<(const NetAddress& lhs, const NetAddress& rhs) {
  return lhs.ToSockAddr().sin_addr.s_addr < rhs.ToSockAddr().sin_addr.s_addr;
}

NetAddress::NetAddress() : port_(0) {}

NetAddress::NetAddress(const alpha::Slice& ip, int port)
    : ip_(ip.ToString()), port_(port) {}

NetAddress::NetAddress(const sockaddr_in& sa) { CHECK(FromSockAddr(sa)); }

std::string NetAddress::FullAddress() const {
  // 255.255.255.255:65535
  if (unlikely(addr_.empty())) {
    char buf[30];
    snprintf(buf, sizeof(buf), "%s:%d", ip_.c_str(), port_);
    addr_ = buf;
  }
  return addr_;
}

bool NetAddress::FromSockAddr(const sockaddr_in& sa) {
  char buf[INET_ADDRSTRLEN];
  const char* dst = ::inet_ntop(AF_INET, &(sa.sin_addr), buf, sizeof(buf));
  if (dst == nullptr) {
    return false;
  }
  port_ = ntohs(sa.sin_port);
  ip_ = buf;
  addr_.clear();
  return true;
}

sockaddr_in NetAddress::ToSockAddr() const {
  sockaddr_in sa;
  memset(&sa, 0x0, sizeof(sa));

  sa.sin_family = AF_INET;
  sa.sin_port = htons(port_);
  int ret = ::inet_pton(AF_INET, ip_.c_str(), &(sa.sin_addr));
  PCHECK(ret == 1) << "inet_pton failed, ret = " << ret
                   << ", addr = " << FullAddress();

  return sa;
}

NetAddress NetAddress::GetLocalAddr(int sockfd) {
  NetAddress addr;
  bool ok = GetLocalAddr(sockfd, &addr);
  (void)ok;
  return addr;
}

NetAddress NetAddress::GetPeerAddr(int sockfd) {
  NetAddress addr;
  bool ok = GetPeerAddr(sockfd, &addr);
  (void)ok;
  return addr;
}

bool NetAddress::GetLocalAddr(int sockfd, NetAddress* addr) {
  sockaddr_in sa;
  memset(&sa, 0x0, sizeof(sa));
  socklen_t sock_len = sizeof(sa);

  int err = ::getsockname(sockfd, reinterpret_cast<sockaddr*>(&sa), &sock_len);
  if (err) {
    return false;
  } else {
    *addr = NetAddress(sa);
    return true;
  }
}

bool NetAddress::GetPeerAddr(int sockfd, NetAddress* addr) {
  sockaddr_in sa;
  memset(&sa, 0x0, sizeof(sa));
  socklen_t sock_len = sizeof(sa);

  int err = ::getpeername(sockfd, reinterpret_cast<sockaddr*>(&sa), &sock_len);
  if (err) {
    return false;
  } else {
    *addr = NetAddress(sa);
    return true;
  }
}
}
