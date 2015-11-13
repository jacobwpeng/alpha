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
#include "Frame.h"
#include "MethodPayloadCodec.h"

namespace amqp {

class CodedWriterBase;
class CodecEnv;
class FSM {
 public:
  FSM(CodedWriterBase* w, const CodecEnv* env);
  virtual ~FSM() = default;
  void set_codec_env(const CodecEnv* env);
  // virtual void HandleFrame(FramePtr&& frame) = 0;

 protected:
  CodedWriterBase* w_;
  const CodecEnv* codec_env_;
};

class ConnectionEstablishState;
using ConnectionEstablishStatePtr = std::unique_ptr<ConnectionEstablishState>;

class ConnectionEstablishState {
 public:
  virtual ~ConnectionEstablishState() = default;
  bool HandleFrame(FramePtr&& frame);
  bool WriteReply();

 protected:
  ConnectionEstablishState(CodedWriterBase* w);
  void AddEncodeUnit(EncoderBase* e);
  void AddDecodeUnit(DecoderBase* e);

  CodedWriterBase* w_;
  std::vector<EncoderBase*> encoders_;
  std::vector<DecoderBase*> decoders_;
};

class ConnectionEstablishStart : public ConnectionEstablishState {
 public:
  ConnectionEstablishStart(CodedWriterBase* w, const CodecEnv* codec_env,
                           const MethodStartOkArgs& start_ok_args);

 private:
  amqp::MethodStartOkArgsEncoder e_;
  amqp::MethodStartArgsDecoder d_;
};

class ConnectionEstablishTune : public ConnectionEstablishState {
 public:
  ConnectionEstablishTune(CodedWriterBase* w);
  virtual bool HandleFrame(FramePtr&& frame);
};

class ConnectionEstablishFSM : public FSM {
 public:
  enum class Status : uint8_t {
    kDone = 0,
    kWaitForWrite = 1,
    kWaitMoreFrame = 2
  };
  ConnectionEstablishFSM(CodedWriterBase* w, const CodecEnv* env);
  Status HandleFrame(FramePtr&& frame);
  bool FlushReply();

 private:
  enum class State : uint8_t {
    kDone = 0,
    kWaitingStart = 1,
    kWaitingTune = 2,
    kWaitingOpenOk = 3
  };

  std::unique_ptr<ConnectionEstablishState> SwitchState(State current_state);
  std::string user_;
  std::string passwd_;
  State state_;
  std::unique_ptr<ConnectionEstablishState> state_handler_;
  amqp::MethodStartOkArgs start_ok_args_;
};
}

#endif /* ----- #ifndef __CONNECTIONESTABLISHFSM_H__  ----- */
