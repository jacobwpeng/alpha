/*
 * =============================================================================
 *
 *       Filename:  ConnectionEstablishFSM.h
 *        Created:  11/11/15 10:49:57
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __CONNECTIONESTABLISHFSM_H__
#define __CONNECTIONESTABLISHFSM_H__

#include <memory>
#include <vector>
#include "FSM.h"
#include "MethodPayloadCodec.h"
#include "ConnectionParameters.h"
#include "BasicAuthorization.h"

namespace amqp {

class EncoderBase;
class DecoderBase;
class ConnectionEstablishState;
using ConnectionEstablishStatePtr = std::unique_ptr<ConnectionEstablishState>;

class ConnectionEstablishState final {
 public:
  ConnectionEstablishState(const CodecEnv* codec_env);
  bool HandleFrame(FramePtr&& frame, SendReplyFunc send_reply_func);

  template <typename DecoderType>
  void set_decoder();
  template <typename EncoderType, typename Args>
  void add_encoder(Args&& args);

 protected:
  const CodecEnv* codec_env_;
  std::unique_ptr<DecoderBase> decoder_;
  std::vector<std::unique_ptr<EncoderBase>> encoders_;
};

class ConnectionEstablishFSM : public FSM {
 public:
  ConnectionEstablishFSM(const CodecEnv* env,
                         const PlainAuthorization& auth,
                         const ConnectionParameters& params);
  virtual bool done() const override;
  virtual Status HandleFrame(FramePtr&& frame,
                             SendReplyFunc send_reply_func) override;

 private:
  enum class State : uint8_t {
    kDone = 0,
    kWaitingStart = 1,
    kWaitingTune = 2,
    kWaitingOpenOk = 3
  };
  std::unique_ptr<ConnectionEstablishState> CreateStateHandler();
  std::unique_ptr<ConnectionEstablishState> SwitchState();

  State state_;
  std::unique_ptr<ConnectionEstablishState> state_handler_;
  PlainAuthorization auth_;
  ConnectionParameters params_;
};

template <typename DecoderType>
void ConnectionEstablishState::set_decoder() {
  decoder_ = alpha::make_unique<DecoderType>(codec_env_);
}

template <typename EncoderType, typename Args>
void ConnectionEstablishState::add_encoder(Args&& args) {
  encoders_.emplace_back(
      alpha::make_unique<EncoderType>(std::forward<Args&&>(args), codec_env_));
}
}

#endif /* ----- #ifndef __CONNECTIONESTABLISHFSM_H__  ----- */
