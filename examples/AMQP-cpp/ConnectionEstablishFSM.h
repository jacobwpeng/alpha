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

namespace amqp {

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
  std::vector<FramePacker> frame_packers_;
  std::vector<DecoderBase*> decoders_;
};

class ConnectionEstablishStart : public ConnectionEstablishState {
 public:
  ConnectionEstablishStart(CodedWriterBase* w, const CodecEnv* codec_env,
                           const MethodStartOkArgs& start_ok_args);

 private:
  MethodStartOkArgsEncoder e_;
  MethodStartArgsDecoder d_;
};

class ConnectionEstablishTune : public ConnectionEstablishState {
 public:
  ConnectionEstablishTune(CodedWriterBase* w, const CodecEnv* codec_env,
                          const MethodTuneOkArgs& tune_ok_args,
                          const MethodOpenArgs& open_args);

  void PrintServerTune() const;

 private:
  MethodTuneOkArgsEncoder e_;
  MethodTuneArgsDecoder d_;
  MethodOpenArgsEncoder open_args_encoder_;
};

class ConnectionEstablishOpenOk : public ConnectionEstablishState {
 public:
  ConnectionEstablishOpenOk(CodedWriterBase* w, const CodecEnv* codec_env);

 private:
  MethodOpenOkArgsDecoder d_;
};

class ConnectionEstablishFSM : public FSM {
 public:
  ConnectionEstablishFSM(CodedWriterBase* w, const CodecEnv* env);
  virtual Status HandleFrame(FramePtr&& frame) override;
  virtual bool FlushReply() override;
  virtual bool WriteInitialRequest() override;

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
  MethodStartOkArgs start_ok_args_;
  MethodTuneOkArgs tune_ok_args_;
  MethodOpenArgs open_args_;
};
}

#endif /* ----- #ifndef __CONNECTIONESTABLISHFSM_H__  ----- */
