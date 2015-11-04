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

Frame::Frame(Frame::Type type, ChannelID channel_id,
    uint32_t expected_payload_size)
  :type_(type),
  channel_id_(channel_id),
  expected_payload_size_(expected_payload_size){
}

void Frame::AddPayload(alpha::Slice partial_payload) {
  payload_.append(partial_payload.data(), partial_payload.size());
}

void Frame::swap(Frame& other) {
  std::swap(type_, other.type_);
  std::swap(channel_id_, other.channel_id_);
  std::swap(expected_payload_size_, other.expected_payload_size_);
  std::swap(payload_, other.payload_);
}

Frame::Type Frame::type() const {
  return type_;
}

ChannelID Frame::channel_id() const {
  return channel_id_;
}

bool Frame::global_to_connection() const {
  return channel_id_ == 0;
}

bool Frame::payload_all_received() const {
  return payload_size() == expected_payload_size();
}

size_t Frame::expected_payload_size() const {
  return expected_payload_size_;
}

size_t Frame::payload_size() const {
  return payload_.size();
}

std::string Frame::payload() const {
  return payload_;
}

}
