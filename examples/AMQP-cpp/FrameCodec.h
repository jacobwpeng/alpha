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
#include <alpha/slice.h>
#include <alpha/tcp_connection.h>
#include "MethodArgTypes.h"
#include "Frame.h"

namespace amqp {

class CodedWriterBase;
class EncoderBase;
class FramePacker {
 public:
  FramePacker(ChannelID channel_id, Frame::Type type, EncoderBase* e);

  bool WriteTo(CodedWriterBase* w);

 private:
  bool frame_header_done_;
  bool frame_payload_done_;
  bool frame_end_done_;
  ChannelID channel_id_;
  Frame::Type frame_type_;
  EncoderBase* e_;
};

class FrameReader {
 public:
  FramePtr Read(alpha::Slice& data);

 private:
  FramePtr frame_;
};

class FrameCodec {
 public:
  FramePtr Process(alpha::Slice& data);

 private:
  FramePtr frame_;
};
}

#endif /* ----- #ifndef __FRAMECODEC_H__  ----- */
