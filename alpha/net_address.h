/*
 * =============================================================================
 *
 *       Filename:  net_address.h
 *        Created:  04/05/15 11:24:54
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __NET_ADDRESS_H__
#define __NET_ADDRESS_H__

#include <iosfwd>
#include <string>
#include "slice.h"

struct sockaddr_in;

namespace alpha {
class NetAddress {
 public:
  NetAddress();
  /* IPv4 addr only */
  NetAddress(const Slice& ip, int port);
  NetAddress(const sockaddr_in& sa);

  std::string ip() const { return ip_; }
  int port() const { return port_; }
  std::string FullAddress() const;

  bool FromSockAddr(const sockaddr_in& sa);
  sockaddr_in ToSockAddr() const;

  /* Get local/peer addr from connected sockfd */
  /* Deprecated, use bool Get*Addr(int sockfd, NetAddress*) instead */
  static NetAddress GetLocalAddr(int sockfd);
  static NetAddress GetPeerAddr(int sockfd);

  static bool GetLocalAddr(int sockfd, NetAddress* addr);
  static bool GetPeerAddr(int sockfd, NetAddress* addr);

 private:
  std::string ip_;
  int port_;
  mutable std::string addr_;
};
std::ostream& operator<<(std::ostream& os, const NetAddress& addr);
bool operator==(const NetAddress& lhs, const NetAddress& rhs);
bool operator<(const NetAddress& lhs, const NetAddress& rhs);
}

#endif /* ----- #ifndef __NET_ADDRESS_H__  ----- */
