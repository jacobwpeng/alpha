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

#pragma once

#include "FSM.h"
#include "MethodPayloadCodec.h"

namespace amqp {

class ConnectionCloseFSM final : public FSM {
 public:
  static std::unique_ptr<ConnectionCloseFSM> ActiveClose(
      const CodecEnv* codec_env,
      SendReplyFunc send_reply_func,
      const MethodCloseArgs& close_args);
  static std::unique_ptr<ConnectionCloseFSM> PassiveClose(
      const CodecEnv* codec_env, SendReplyFunc send_reply_func);

  virtual bool done() const override;
  virtual Status HandleFrame(FramePtr&& frame,
                             SendReplyFunc send_reply_func) override;

 private:
  enum class State : uint8_t {
    kDone = 0,
    kWaitingCloseOk = 1,
  };
  ConnectionCloseFSM(const CodecEnv* codec_env);

  State state_;
  GenericMethodArgsDecoder generic_decoder_;
};
}

