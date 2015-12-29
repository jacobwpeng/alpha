/*
 * =============================================================================
 *
 *       Filename:  UDPSocket.h
 *        Created:  12/29/15 10:16:20
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include <alpha/net_address.h>
#include <alpha/IOBuffer.h>

namespace alpha {
class UDPSocket {
 public:
  UDPSocket();
  virtual ~UDPSocket();
  int Open();
  int Connect(const NetAddress& addr);
  int Bind(const NetAddress& addr);
  void Close();
  int GetPeerAddress(NetAddress* addr);
  int GetLocalAddress(NetAddress* addr);

  int Read(IOBuffer* buf, size_t buf_len);
  int Write(IOBuffer* buf, size_t buf_len);

  int RecvFrom(IOBuffer* buf, size_t buf_len, NetAddress* addr);
  int SendTo(IOBuffer* buf, size_t buf_len, const NetAddress& addr);

  int SetReceiveBufferSize(int32_t size);
  int SetSendBufferSize(int32_t size);

  bool is_connected() const { return is_connected_; }
  int fd() const { return socket_; }

 private:
  static const int kInvalidSocket = -1;
  int DoBind(const NetAddress& addr);
  int SendOrWrite(IOBuffer* buf, size_t buf_len, const NetAddress* addr);
  int socket_{kInvalidSocket};
  bool is_connected_{false};
};
}
