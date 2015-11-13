/*
 * =============================================================================
 *
 *       Filename:  ConnectionEstablishFSM.cc
 *        Created:  11/11/15 15:06:57
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "ConnectionEstablishFSM.h"
#include <alpha/compiler.h>
#include "CodedWriter.h"
#include "CodecEnv.h"

namespace amqp {
FSM::FSM(CodedWriterBase* w, const CodecEnv* env) : w_(w), codec_env_(env) {}

void FSM::set_codec_env(const CodecEnv* env) { codec_env_ = env; }

ConnectionEstablishFSM::ConnectionEstablishFSM(CodedWriterBase* w,
                                               const CodecEnv* env)
    : FSM(w, env) {
  std::string protocol_header = {'A', 'M', 'Q', 'P', 0, 0, 9, 1};
  bool done = w_->Write(protocol_header.data(), protocol_header.size());
  // TODO: throw a ConnectionException
  CHECK(done) << "Write protocol header failed";

  start_ok_args_.mechanism = "PLAIN";
  start_ok_args_.locale = "en_US";
  start_ok_args_.response = '\0' + user_ + '\0' + passwd_;
  start_ok_args_.client_properties.Insert(
      "Product", FieldValue(FieldValue::Type::kShortString, "AMQP-cpp"));
  start_ok_args_.client_properties.Insert(
      "Version", FieldValue(FieldValue::Type::kShortString, "0.01"));
  state_ = State::kWaitingStart;
  state_handler_ =
      alpha::make_unique<ConnectionEstablishStart>(w, env, start_ok_args_);
}

ConnectionEstablishState::ConnectionEstablishState(CodedWriterBase* w)
    : w_(w) {}

void ConnectionEstablishState::AddEncodeUnit(EncoderBase* e) {
  encoders_.push_back(e);
}

void ConnectionEstablishState::AddDecodeUnit(DecoderBase* d) {
  decoders_.push_back(d);
}

bool ConnectionEstablishState::HandleFrame(FramePtr&& frame) {
  while (!decoders_.empty()) {
    auto it = decoders_.begin();
    auto decoder = *it;
    int rc = decoder->Decode(frame->payload());
    if (rc == DecodeState::kDone) {
      decoders_.erase(it);
      continue;
    } else if (rc == DecodeState::kNeedsMore) {
      return false;
    } else {
      // TODO: throw a ConnectionException
      CHECK(false);
    }
  }
  return true;
}

bool ConnectionEstablishState::WriteReply() {
  CHECK(decoders_.empty()) << "Write reply before all decoders are done";
  while (!encoders_.empty()) {
    auto it = encoders_.begin();
    auto encoder = *it;
    DLOG_INFO << "encoder->ByteSize() = " << encoder->ByteSize();
    bool done = encoder->Encode(w_);
    if (done) {
      encoders_.erase(it);
    } else {
      return false;
    }
  }
  return true;
}

ConnectionEstablishStart::ConnectionEstablishStart(
    CodedWriterBase* w, const CodecEnv* codec_env,
    const MethodStartOkArgs& start_ok_args)
    : ConnectionEstablishState(w), e_(start_ok_args, codec_env), d_(codec_env) {
  AddEncodeUnit(&e_);
  AddDecodeUnit(&d_);
}

ConnectionEstablishTune::ConnectionEstablishTune(CodedWriterBase* w)
    : ConnectionEstablishState(w) {}

bool ConnectionEstablishTune::HandleFrame(FramePtr&& frame) { return false; }

ConnectionEstablishFSM::Status ConnectionEstablishFSM::HandleFrame(
    FramePtr&& frame) {
  bool done = state_handler_->HandleFrame(std::move(frame));
  if (done) {
    done = state_handler_->WriteReply();
    if (done) {
      state_handler_ = SwitchState(state_);
      return state_handler_ ? Status::kWaitMoreFrame : Status::kDone;
    } else {
      return Status::kWaitForWrite;
    }
  } else {
    return Status::kWaitMoreFrame;
  }
}

bool ConnectionEstablishFSM::FlushReply() {
  CHECK(state_handler_);
  return state_handler_->WriteReply();
}

std::unique_ptr<ConnectionEstablishState> ConnectionEstablishFSM::SwitchState(
    State current_state) {
  DLOG_INFO << "SwitchState current = " << static_cast<int>(current_state);
  switch (current_state) {
    case State::kWaitingStart:
      state_ = State::kWaitingTune;
      return alpha::make_unique<ConnectionEstablishTune>(w_);
    default:
      CHECK(false);
      break;
  }
  return nullptr;
}
}
