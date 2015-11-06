/*
 * =============================================================================
 *
 *       Filename:  FrameCodec.cc
 *        Created:  10/21/15 14:38:14
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include "FrameCodec.h"
#include <alpha/logger.h>
#include "Frame.h"
#include "CodedInputStream.h"

namespace amqp {
  static const size_t kFrameHeaderSize = 7;

  FramePtr FrameCodec::Process(alpha::Slice& data) {
    if (frame_ == nullptr) {
      if (data.size() < kFrameHeaderSize) {
        return nullptr; //needs more data to proceed
      } else {
        CodedInputStream stream(data);
        uint8_t frame_type;
        ChannelID frame_channel;
        uint32_t frame_payload_size;
        stream.ReadUInt8(&frame_type);
        stream.ReadBigEndianUInt16(&frame_channel);
        stream.ReadBigEndianUInt32(&frame_payload_size);
        // TODO: Throw a ConnectionException
        CHECK(Frame::ValidType(frame_type)) << "Invalid frame type: "
          << frame_type;;
        frame_.reset(new Frame(static_cast<Frame::Type>(frame_type),
                               frame_channel,
                               frame_payload_size
                             )
            );
        data.Advance(stream.consumed_bytes());
      }
    }
    CHECK(frame_);
    DLOG_INFO << "Expected frame size = " << frame_->expected_payload_size()
      << ", current frame size = " << frame_->payload_size()
      << ", data.size() = " << data.size();
    size_t sz = 0;
    if (frame_->payload_all_received()) {
      // Waiting for frame end
      sz = std::min<size_t>(1, data.size());
    } else {
      sz = std::min(frame_->expected_payload_size(), data.size());
    }
    frame_->AddPayload(data.subslice(0, sz));
    data.Advance(sz);
    if (frame_->payload_all_received()) {
      // All payloads arrived, check frame end
      static const char kFrameEnd = 0xCE;
      if (!data.empty() && *data.data() == kFrameEnd) {
        // Valid frame, return it
        data.Advance(1);
        return std::move(frame_);
      } else if (!data.empty()) {
        // Invalid frame end, log first
        LOG_WARNING << "Invalid frame end: " << static_cast<int>(*data.data());
        // TODO: Throw a ConnectionException
        CHECK(false);
      } else {
        // Waiting for frame end
      }
    }
    return nullptr;
  }
}