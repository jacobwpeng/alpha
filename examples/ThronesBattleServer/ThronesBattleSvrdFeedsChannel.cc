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
#include <alpha/Random.h>
#include <alpha/Logger.h>
#include "proto/feedssvrd.pb.h"

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

void FeedsChannel::AddFightMessage(unsigned msg_type,
                                   const google::protobuf::Message* m) {
  FeedsServerProtocol::Task task;
  task.set_msg_type(msg_type);
  bool ok = m->SerializeToString(task.mutable_payload());
  CHECK(ok) << "Serialize fight message proto failed";
  auto frame = NetSvrdFrame::CreateUnique(task.ByteSize());
  ok = task.SerializeToArray(frame->payload, frame->payload_size);
  CHECK(ok) << "Serialize fight message proto failed";
  SendToRemote(std::move(frame));
}

void FeedsChannel::WaitAllFeedsSended() {
  auto r = alpha::Random::Rand32();
  try {
    DLOG_INFO << "Will wait all message sended, r: " << r;
    conn_->WaitWriteDone();
    DLOG_INFO << "Wait all message sended done, r: " << r;
  } catch (alpha::AsyncTcpConnectionException& e) {
    LOG_WARNING << "Catch exception when wait write done, " << e.what();
  }
}

FeedsChannel::~FeedsChannel() {
  if (conn_) {
    conn_->Close();
    conn_.reset();
  }
}

void FeedsChannel::SendToRemote(NetSvrdFrame::UniquePtr&& frame) {
  // alpha::WrappedIOBuffer buf(frame->data());
  // socket_.Write(&buf, frame->size());
  if (conn_ == nullptr) {
    ReconnectToRemote();
  }
  try {
    conn_->Write(frame->data(), frame->size());
  } catch (alpha::AsyncTcpConnectionException& e) {
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
