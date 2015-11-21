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
#include "CodedWriter.h"
#include "MethodPayloadCodec.h"
#include "CodedOutputStream.h"

namespace amqp {
static const size_t kFrameHeaderSize = 7;

FramePacker::FramePacker(ChannelID channel_id, Frame::Type frame_type,
                         EncoderBase* e)
    : frame_header_done_(false),
      frame_payload_done_(false),
      frame_end_done_(false),
      channel_id_(channel_id),
      frame_type_(frame_type),
      e_(e) {}

bool FramePacker::WriteTo(CodedWriterBase* w) {
  CodedOutputStream stream(w);
  if (!frame_header_done_ && w->CanWrite(7)) {
    stream.WriteUInt8(frame_type_);
    stream.WriteBigEndianUInt16(channel_id_);
    stream.WriteBigEndianUInt32(e_->ByteSize());
    frame_header_done_ = true;
  }
  if (!frame_payload_done_) {
    frame_payload_done_ = e_->Encode(w);
  }
  if (frame_payload_done_ && !frame_end_done_ && w->CanWrite(1)) {
    stream.WriteUInt8(0xCE);
    frame_end_done_ = true;
  }
  return true;
}

FramePtr FrameReader::Read(alpha::Slice& data) {
  if (frame_ == nullptr) {
    if (data.size() < kFrameHeaderSize) {
      return nullptr;  // needs more data to proceed
    } else {
      CodedInputStream stream(data);
      uint8_t frame_type;
      ChannelID frame_channel;
      uint32_t frame_payload_size;
      stream.ReadUInt8(&frame_type);
      stream.ReadBigEndianUInt16(&frame_channel);
      stream.ReadBigEndianUInt32(&frame_payload_size);
      // TODO: Throw a ConnectionException
      CHECK(Frame::ValidType(frame_type))
          << "Invalid frame type: " << frame_type;
      ;
      frame_.reset(new Frame(static_cast<Frame::Type>(frame_type),
                             frame_channel, frame_payload_size));
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

void FrameReader::Reset() { frame_.reset(); }
}
