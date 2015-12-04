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

#include "Channel.h"
#include <alpha/compiler.h>
#include <alpha/logger.h>
#include "Connection.h"

namespace amqp {
Channel::Channel(ChannelID channel_id, const std::shared_ptr<Connection>& conn)
    : closed_(false), id_(channel_id), conn_(conn) {}

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

void Channel::Close() {
  if (closed()) {
    return;
  }
  if (auto conn = conn_.lock()) {
    conn->CloseChannel(id_);
  }
}
}
