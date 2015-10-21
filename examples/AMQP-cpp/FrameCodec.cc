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
  FrameCodec::FrameCodec()
    :frame_(nullptr) {
  }

  size_t FrameCodec::Process(alpha::Slice data) {
    if (frame_ == nullptr) {
      if (data.size() < kFrameHeaderSize) {
        return 0; //needs more data to proceed
      } else {
        CodedInputStream stream(data);
        uint8_t frame_type;
        ChannelID frame_channel;
        uint32_t frame_payload_size;
        stream.ReadUInt8(&frame_type);
        stream.ReadBigEndianUInt16(&frame_channel);
        stream.ReadBigEndianUInt32(&frame_payload_size);
        CHECK(Frame::ValidType(frame_type)) << "Invalid frame type: "
          << frame_type;;
        frame_ = new Frame(static_cast<Frame::Type>(frame_type), frame_channel,
            frame_payload_size);
        data.Advance(stream.consumed_bytes());
      }
    }
    CHECK(frame_);
    DLOG_INFO << "Expected frame size = " << frame_->expeced_payload_size()
      << ", current frame size = " << frame_->payload_size()
      << ", data.size() = " << data.size();
    size_t sz = std::min(frame_->expeced_payload_size(), data.size());
    frame_->AddPayload(data.subslice(0, sz));
    data.Advance(sz);
    if (frame_->expeced_payload_size() == frame_->payload_size()) {
      // All payloads arrived, check frame end
      static const char kFrameEnd = 0xCE;
      if (!data.empty() && *data.data() == kFrameEnd) {
        // Valid frame, just tell callback if it exists
        if (new_frame_callback_) new_frame_callback_(frame_);
        // Delete frame anyway
        // client could use frame_->Swap(saved) to save this frame if necessary
        delete frame_;
      } else if (!data.empty()) {
        // Invalid frame end, log first
        LOG_WARNING << "Invalid frame end: " << static_cast<int>(*data.data());
        // Then abort this connection
        CHECK(false);
      } else {
        // Waiting for frame end
      }
    }
    return sz;
  }

  void FrameCodec::SetNewFrameCallback(const NewFrameCallback& cb) {
    new_frame_callback_ = cb;
  }
}
