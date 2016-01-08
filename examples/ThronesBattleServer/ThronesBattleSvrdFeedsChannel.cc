/*
 * =============================================================================
 *
 *       Filename:  ThronesBattleSvrdFeedsChannel.cc
 *        Created:  01/05/16 16:46:38
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "ThronesBattleSvrdFeedsChannel.h"
#include <alpha/AsyncTcpConnectionException.h>

namespace ThronesBattle {
FeedsChannel::FeedsChannel(alpha::AsyncTcpClient* async_tcp_client,
                           alpha::AsyncTcpConnectionCoroutine* co,
                           const alpha::NetAddress& address)
    : async_tcp_client_(async_tcp_client), co_(co), address_(address) {
  auto err = socket_.Open();
  PCHECK(!err) << "Open failed";
  err = socket_.Connect(address_);
  PCHECK(err == 0) << "Connect failed";
}

FeedsChannel::~FeedsChannel() {
  if (conn_) {
    conn_->Close();
    conn_.reset();
  }
}

std::string FeedsChannel::CreateParams(std::ostringstream& oss) {
  return oss.str();
}

void FeedsChannel::SendToRemote(NetSvrdFrame::UniquePtr&& frame) {
  // alpha::WrappedIOBuffer buf(frame->data());
  // socket_.Write(&buf, frame->size());
  if (conn_ == nullptr) {
    ReconnectToRemote();
  }
  try {
    conn_->Write(frame->data(), frame->size());
  }
  catch (alpha::AsyncTcpConnectionException& e) {
    ReconnectToRemote();
  }
}

void FeedsChannel::ReconnectToRemote() {
  if (conn_) {
    conn_->Close();
    conn_.reset();
  }

  do {
    conn_ = async_tcp_client_->ConnectTo(address_, co_);
    if (conn_) break;
    LOG_WARNING << "ConnectTo " << address_ << " failed, retry after "
                << kReconnectInterval << "(ms)";
    co_->YieldWithTimeout(kReconnectInterval);
  } while (conn_ == nullptr);
}
}
