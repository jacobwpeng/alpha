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
  user_ = "guest";
  passwd_ = "guest";

  start_ok_args_.mechanism = "PLAIN";
  start_ok_args_.locale = "en_US";
  start_ok_args_.response.push_back('\0');
  start_ok_args_.response.append(user_);
  start_ok_args_.response.push_back('\0');
  start_ok_args_.response.append(passwd_);
  start_ok_args_.client_properties.Insert(
      "Product", FieldValue(FieldValue::Type::kShortString, "AMQP-cpp"));
  start_ok_args_.client_properties.Insert(
      "Version", FieldValue(FieldValue::Type::kShortString, "0.01"));
  state_ = State::kWaitingStart;
  state_handler_ = alpha::make_unique<ConnectionEstablishStart>(w_, codec_env_,
                                                                start_ok_args_);
}

ConnectionEstablishState::ConnectionEstablishState(CodedWriterBase* w)
    : w_(w) {}

void ConnectionEstablishState::AddEncodeUnit(EncoderBase* e) {
  frame_packers_.emplace_back(0, Frame::Type::kMethod, e);
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
  while (!frame_packers_.empty()) {
    auto it = frame_packers_.begin();
    if (it->WriteTo(w_)) {
      DLOG_INFO << "One FramePacker done";
      frame_packers_.erase(it);
      continue;
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

ConnectionEstablishTune::ConnectionEstablishTune(
    CodedWriterBase* w, const CodecEnv* codec_env,
    const MethodTuneOkArgs& tune_ok_args)
    : ConnectionEstablishState(w), e_(tune_ok_args, codec_env), d_(codec_env) {
  AddEncodeUnit(&e_);
  AddDecodeUnit(&d_);
}

void ConnectionEstablishTune::PrintServerTune() const {
  // auto args = d_.Get();
  auto args = d_.GetArg<MethodTuneArgs>();
  DLOG_INFO << "Channel-Max: " << args.channel_max
            << ", Frame-Max: " << args.frame_max
            << ", Heartbeat-Delay: " << args.heartbeat_delay;
}

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
      tune_ok_args_.channel_max = 1;
      tune_ok_args_.frame_max = 1 << 20;
      tune_ok_args_.heartbeat_delay = 1 << 10;
      return alpha::make_unique<ConnectionEstablishTune>(w_, codec_env_,
                                                         tune_ok_args_);
    case State::kWaitingTune:
      dynamic_cast<ConnectionEstablishTune*>(state_handler_.get())
          ->PrintServerTune();
      return nullptr;
    default:
      CHECK(false) << "Invalid current_state = "
                   << static_cast<int>(current_state);
      break;
  }
  return nullptr;
}
}
