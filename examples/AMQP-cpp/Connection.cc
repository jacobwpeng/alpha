/*
 * =============================================================================
 *
 *       Filename:  Connection.cc
 *        Created:  11/15/15 17:14:27
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "Connection.h"
#include "Channel.h"
#include "ConnectionMgr.h"

namespace amqp {
Connection::Connection(ConnectionMgr* owner, alpha::TcpConnectionPtr& conn)
    : next_channel_id_(1), owner_(owner), conn_(conn) {}

Connection::~Connection() = default;

void Connection::Close() { owner_->CloseConnection(this); }

bool Connection::HandleFrame(FramePtr&& frame) { return false; }

void Connection::NewChannel() {}

void Connection::CloseChannel(ChannelID channel_id) {}

alpha::TcpConnectionPtr Connection::tcp_connection() const {
  return conn_.lock();
}

void Connection::OnChannelOpened(ChannelID channel_id) {
  auto p = channels_.emplace(
      channel_id, alpha::make_unique<Channel>(channel_id, shared_from_this()));
  CHECK(p.second) << "Channel opened twice, channel_id: " << channel_id;
}
}
