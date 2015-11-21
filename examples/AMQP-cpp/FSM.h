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

#include "Frame.h"
#include "FrameCodec.h"

namespace amqp {

class CodedWriterBase;
class CodecEnv;

class FSM {
 public:
  enum class Status : uint8_t {
    kDone = 0,
    kConnectionEstablished = 0,
    kWaitForWrite = 1,
    kWaitMoreFrame = 2
  };

  FSM(CodedWriterBase* w, const CodecEnv* env);
  virtual ~FSM() = default;
  void set_codec_env(const CodecEnv* env);
  virtual Status HandleFrame(FramePtr&& frame) = 0;
  virtual bool FlushReply() = 0;
  virtual bool WriteInitialRequest() = 0;

 protected:
  CodedWriterBase* w_;
  const CodecEnv* codec_env_;
};
}

#endif /* ----- #ifndef __FSM_H__  ----- */
