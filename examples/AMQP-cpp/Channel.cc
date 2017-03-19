/*
 * =============================================================================
 *
 *       Filename:  Channel.cc
 *        Created:  11/22/15 16:12:15
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <alpha/Channel.h>
#include <alpha/Compiler.h>
#include <alpha/Logger.h>
#include "Connection.h"
#include "MethodArgs.h"

namespace amqp {
Channel::Channel(ChannelID channel_id, const std::shared_ptr<Connection>& conn)
    : closed_(false), id_(channel_id), conn_(conn) {}

void Channel::Close() {
  if (closed()) {
    return;
  }
  if (auto conn = conn_.lock()) {
    conn->CloseChannel(id_);
  }
}

void Channel::AddCachedFrame(FramePtr&& frame) {
  if (unlikely(cached_frames_.size() == kMaxCachedFrameNum)) {
    CHECK(false) << "Too many cached frames, channel id: " << id_;
  }
  cached_frames_.emplace(std::move(frame));
}

FramePtr Channel::PopCachedFrame() {
  if (cached_frames_.empty()) {
    return nullptr;
  }
  auto frame = std::move(cached_frames_.front());
  cached_frames_.pop();
  return std::move(frame);
}

std::shared_ptr<Connection> Channel::CheckConnection() { return conn_.lock(); }

void Channel::ExchangeDeclare(alpha::Slice exchange_name,
                              alpha::Slice exchange_type,
                              bool passive,
                              bool durable,
                              bool auto_delete,
                              const FieldTable& arguments) {
  MethodExchangeDeclareArgs args;
  args.ticket = 0;
  args.exchange = exchange_name.ToString();
  args.type = exchange_type.ToString();
  args.passive = passive;
  args.durable = durable;
  args.auto_delete = auto_delete;
  args.arguments = arguments;
  CheckConnection()->WriteMethod(id(), args);
}
}
