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
#include <alpha/AsyncTcpConnection.h>
#include "Frame.h"
#include "CodedInputStream.h"
#include "CodedWriter.h"
#include "MethodPayloadCodec.h"
#include "CodedOutputStream.h"

namespace amqp {
static const size_t kFrameHeaderSize = 7;

#if 0
FramePacker::FramePacker(ChannelID channel_id, Frame::Type frame_type,
                         std::unique_ptr<EncoderBase> e)
    : frame_header_done_(false),
      frame_payload_done_(false),
      frame_end_done_(false),
      channel_id_(channel_id),
      frame_type_(frame_type),
      e_(std::move(e)) {}

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
#endif
FrameWriter::FrameWriter(alpha::AsyncTcpConnection* conn) : conn_(conn) {}

void FrameWriter::WriteMethod(ChannelID channel_id, EncoderBase* e) {
  AsyncTcpConnectionWriter w(conn_);
  CodedOutputStream stream(&w);
  stream.WriteUInt8(Frame::Type::kMethod);
  stream.WriteBigEndianUInt16(channel_id);
  stream.WriteBigEndianUInt32(e->ByteSize());
  e->WriteTo(&w);
  stream.WriteUInt8(kFrameEnd);
}

FrameReader::FrameReader(alpha::AsyncTcpConnection* conn) : conn_(conn) {}

FramePtr FrameReader::Read() {
  auto frame_header = conn_->Read(7);
  CodedInputStream stream(frame_header);
  uint8_t frame_type;
  ChannelID frame_channel;
  uint32_t payload_size;
  stream.ReadUInt8(&frame_type);
  stream.ReadBigEndianUInt16(&frame_channel);
  stream.ReadBigEndianUInt32(&payload_size);
  CHECK(Frame::ValidType(frame_type))
      << "Invalid frame type: " << static_cast<int>(frame_type);
  auto payload = conn_->Read(payload_size);
  CodedInputStream frame_end_stream(conn_->Read(1));
  uint8_t frame_end;
  frame_end_stream.ReadUInt8(&frame_end);
  CHECK(frame_end == kFrameEnd)
      << "Invalid frame end: " << static_cast<int>(frame_end);
  return alpha::make_unique<Frame>(static_cast<Frame::Type>(frame_type),
                                   frame_channel, std::move(payload));
}
}
