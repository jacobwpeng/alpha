/*
 * =============================================================================
 *
 *       Filename:  ConnectionCloseFSM.h
 *        Created:  11/19/15 20:28:41
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __CONNECTIONCLOSEFSM_H__
#define __CONNECTIONCLOSEFSM_H__

#include "FSM.h"
#include "MethodPayloadCodec.h"

namespace amqp {

class ConnectionCloseState {
 public:
  virtual ~ConnectionCloseState() = default;
  virtual bool HandleFrame(FramePtr&& frame) = 0;
  virtual bool WriteReply() = 0;
};

// Accept Both Close/Close-OK methods
class ConnectionCloseFirstStage final : public ConnectionCloseState {
 public:
  ConnectionCloseFirstStage(CodedWriterBase* w, const CodecEnv* codec_env);
  virtual bool HandleFrame(FramePtr&& frame) override;
  virtual bool WriteReply() override;
  bool should_goto_second_stage() const { return should_goto_second_stage_; }

 private:
  bool should_goto_second_stage_;
  CodedWriterBase* w_;
  MethodCloseOkArgsEncoder e_;
  GenericMethodArgsDecoder generic_decoder_;
};

// Accept Close-OK only
class ConnectionCloseSecondStage final : public ConnectionCloseState {
 public:
  ConnectionCloseSecondStage(const CodecEnv* codec_env);
  virtual bool HandleFrame(FramePtr&& frame) override;
  virtual bool WriteReply() override;

 private:
  MethodCloseOkArgsDecoder d_;
};

class ConnectionCloseFSM final : public FSM {
 public:
  ConnectionCloseFSM(CodedWriterBase* w, const CodecEnv* env,
                     const MethodCloseArgs& method_close_args);
  virtual Status HandleFrame(FramePtr&& frame) override;
  virtual bool FlushReply() override;
  virtual bool WriteInitialRequest() override;

 private:
  enum class State : uint8_t { kFirstStage = 1, kSecondStage = 2 };
  State state_;
  std::unique_ptr<ConnectionCloseState> state_handler_;
  GenericMethodArgsDecoder generic_decoder_;
  MethodCloseArgsEncoder method_close_args_encoder_;
  FramePacker frame_packer_;
};
}

#endif /* ----- #ifndef __CONNECTIONCLOSEFSM_H__  ----- */
