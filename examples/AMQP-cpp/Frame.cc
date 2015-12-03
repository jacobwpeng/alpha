/*
 * =============================================================================
 *
 *       Filename:  Frame.cc
 *        Created:  10/21/15 15:17:04
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "Frame.h"

namespace amqp {

bool Frame::ValidType(uint8_t frame_type) {
  switch (frame_type) {
    case Frame::Type::kMethod:
    case Frame::Type::kHeader:
    case Frame::Type::kBody:
    case Frame::Type::kHeartbeat:
      return true;
    default:
      return false;
  }
}

Frame::Frame(Type type, ChannelID channel_id, std::string&& payload)
    : type_(type), channel_id_(channel_id), payload_(std::move(payload)) {}

void Frame::swap(Frame& other) {
  std::swap(type_, other.type_);
  std::swap(channel_id_, other.channel_id_);
  std::swap(payload_, other.payload_);
}

Frame::Type Frame::type() const { return type_; }

ChannelID Frame::channel_id() const { return channel_id_; }

bool Frame::global_to_connection() const { return channel_id_ == 0; }

size_t Frame::payload_size() const { return payload_.size(); }

std::string Frame::payload() const { return payload_; }
}
