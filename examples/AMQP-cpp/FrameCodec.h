/*
 * =============================================================================
 *
 *       Filename:  FrameCodec.h
 *        Created:  10/21/15 14:33:10
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __FRAMECODEC_H__
#define __FRAMECODEC_H__

#include <functional>
#include <alpha/Slice.h>
#include <alpha/TcpConnection.h>
#include "MethodArgTypes.h"
#include "Frame.h"

namespace alpha {
class AsyncTcpConnection;
}

namespace amqp {
class CodedWriterBase;
class EncoderBase;
#if 0
class FramePacker {
 public:
  FramePacker(ChannelID channel_id, Frame::Type type,
              std::unique_ptr<EncoderBase> e);
  bool WriteTo(CodedWriterBase* w);

 private:
  bool frame_header_done_;
  bool frame_payload_done_;
  bool frame_end_done_;
  ChannelID channel_id_;
  Frame::Type frame_type_;
  std::unique_ptr<EncoderBase> e_;
};

class FrameReader {
 public:
  FramePtr Read(alpha::Slice& data);
  void Reset();

 private:
  FramePtr frame_;
};
#endif

class FrameWriter {
 public:
  explicit FrameWriter(alpha::AsyncTcpConnection* conn);
  void WriteMethod(ChannelID channel_id, EncoderBase* e);

 private:
  alpha::AsyncTcpConnection* conn_;
};

class FrameReader {
 public:
  explicit FrameReader(alpha::AsyncTcpConnection* conn);
  FramePtr Read();

 private:
  alpha::AsyncTcpConnection* conn_;
};
}

#endif /* ----- #ifndef __FRAMECODEC_H__  ----- */
