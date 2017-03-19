/*
 * =============================================================================
 *
 *       Filename:  UDPServer.cc
 *        Created:  12/29/15 15:07:05
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <alpha/UDPServer.h>
#include <alpha/Logger.h>
#include <alpha/Channel.h>
#include <alpha/EventLoop.h>

namespace alpha {
UDPServer::UDPServer(EventLoop* loop) : loop_(loop) {}

UDPServer::~UDPServer() = default;

bool UDPServer::Run(const NetAddress& address) {
  int err = socket_.Open();
  if (err) {
    PLOG_ERROR << "UDPSocket::Open";
    return false;
  }
  err = socket_.Bind(address);
  if (err) {
    PLOG_ERROR << "UDPSocket::Bind";
    return false;
  }
  channel_ = make_unique<Channel>(loop_, socket_.fd());
  using namespace std::placeholders;
  channel_->set_read_callback(std::bind(&UDPServer::OnReadable, this));
  channel_->EnableReading();
  channel_->DisableWriting();
  return true;
}

void UDPServer::OnReadable() {
  static const size_t kMaxUDPSize = 1 << 16;
  alpha::IOBuffer buf(kMaxUDPSize);
  alpha::NetAddress peer;
  int nread = socket_.RecvFrom(&buf, kMaxUDPSize, &peer);
  if (nread >= 0) {
    DLOG_INFO << "Read " << nread << " bytes from " << peer;
    if (cb_) {
      cb_(&socket_, &buf, nread, peer);
    }
  } else {
    PLOG_WARNING << "UDPSocket::RecvFrom failed";
  }
}
}
