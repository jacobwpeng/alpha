/*
 * =============================================================================
 *
 *       Filename:  Frame.h
 *        Created:  10/21/15 14:40:41
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __FRAME_H__
#define __FRAME_H__

#include <cstdint>
#include <memory>
#include <alpha/slice.h>
#include "MethodArgTypes.h"

namespace amqp {

class Frame {
 public:
  enum Type : uint8_t { kMethod = 1, kHeader = 2, kBody = 3, kHeartbeat = 4 };
  static bool ValidType(uint8_t frame_type);

  Frame(Type type, ChannelID channel_id, std::string&& payload);
  void swap(Frame& other);

  Type type() const;
  ChannelID channel_id() const;
  bool global_to_connection() const;
  bool payload_all_received() const;
  size_t payload_size() const;
  std::string payload() const;

 private:
  Type type_;
  ChannelID channel_id_;
  std::string payload_;
};
static const uint8_t kFrameEnd = 0xCE;
using FramePtr = std::unique_ptr<Frame>;
}

#endif /* ----- #ifndef __FRAME_H__  ----- */
