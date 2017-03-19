/*
 * =============================================================================
 *
 *       Filename:  FSM.h
 *        Created:  11/19/15 20:26:10
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __FSM_H__
#define __FSM_H__

#include <functional>
#include "Frame.h"
#include "FrameCodec.h"

namespace amqp {

class EncoderBase;
class CodedWriterBase;
class CodecEnv;

using SendReplyFunc =
    std::function<void(ChannelID channel_id,
                       Frame::Type type,
                       std::unique_ptr<EncoderBase>&& encoder)>;
class FSM {
 public:
  enum class Status : uint8_t { kDone = 0, kWaitMoreFrame = 1 };
  FSM(const CodecEnv* env);
  virtual ~FSM() = default;
  void set_codec_env(const CodecEnv* env);

  virtual bool done() const = 0;
  virtual Status HandleFrame(FramePtr&& frame,
                             SendReplyFunc send_reply_func) = 0;

 protected:
  const CodecEnv* codec_env_;
};
}

#endif /* ----- #ifndef __FSM_H__  ----- */
