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
ConnectionEstablishState::ConnectionEstablishState(const CodecEnv* codec_env)
    : codec_env_(codec_env) {}

bool ConnectionEstablishState::HandleFrame(FramePtr&& frame,
                                           SendReplyFunc send_reply_func) {
  int rc = decoder_->Decode(frame->payload());
  if (rc == DecodeState::kDone) {
    std::for_each(encoders_.begin(), encoders_.end(),
                  [&send_reply_func](std::unique_ptr<EncoderBase>& encoder) {
      send_reply_func(0, Frame::Type::kMethod, std::move(encoder));
    });
    return true;
  } else if (rc == DecodeState::kNeedsMore) {
    return false;
  } else {
    // TODO: throw a ConnectionException
    CHECK(false) << "DecodeState::kError";
    return false;
  }
}

ConnectionEstablishFSM::ConnectionEstablishFSM(
    const CodecEnv* codec_env, const PlainAuthorization& auth,
    const ConnectionParameters& params)
    : FSM(codec_env),
      state_(State::kWaitingStart),
      auth_(auth),
      params_(params) {
  MethodStartOkArgs start_ok_args;
  start_ok_args.mechanism = "PLAIN";
  start_ok_args.locale = "en_US";
  start_ok_args.response.push_back('\0');
  start_ok_args.response.append(auth_.user);
  start_ok_args.response.push_back('\0');
  start_ok_args.response.append(auth_.passwd);
  start_ok_args.client_properties.Insert(
      "Product", FieldValue(FieldValue::Type::kShortString, "AMQP-cpp"));
  start_ok_args.client_properties.Insert(
      "Version", FieldValue(FieldValue::Type::kShortString, "0.01"));
  state_handler_ = CreateStateHandler();
  state_handler_->set_decoder<MethodStartArgsDecoder>();
  state_handler_->add_encoder<MethodStartOkArgsEncoder>(start_ok_args);
}

bool ConnectionEstablishFSM::done() const { return state_ == State::kDone; }

FSM::Status ConnectionEstablishFSM::HandleFrame(FramePtr&& frame,
                                                SendReplyFunc send_reply_func) {
  bool done = state_handler_->HandleFrame(std::move(frame), send_reply_func);
  if (!done) {
    DLOG_INFO << "Needs more frame for state handler";
    return Status::kWaitMoreFrame;
  }
  state_handler_ = SwitchState();
  if (state_handler_ == nullptr) {
    return Status::kDone;
  }
  return Status::kWaitMoreFrame;
}

std::unique_ptr<ConnectionEstablishState>
ConnectionEstablishFSM::CreateStateHandler() {
  auto state_handler = alpha::make_unique<ConnectionEstablishState>(codec_env_);
  return std::move(state_handler);
}

std::unique_ptr<ConnectionEstablishState>
ConnectionEstablishFSM::SwitchState() {
  DLOG_INFO << "SwitchState, current state: " << static_cast<int>(state_);
  std::unique_ptr<ConnectionEstablishState> state_handler;
  if (state_ == State::kWaitingStart) {
    MethodTuneOkArgs tune_ok_args;
    tune_ok_args.channel_max = params_.channel_max;
    tune_ok_args.frame_max = params_.frame_max;
    tune_ok_args.heartbeat_delay = params_.heartbeat_delay;

    MethodOpenArgs open_args;
    open_args.vhost_path = params_.vhost;
    open_args.capabilities = false;
    open_args.insist = true;

    state_handler = CreateStateHandler();
    state_handler->set_decoder<MethodTuneArgsDecoder>();
    state_handler->add_encoder<MethodTuneOkArgsEncoder>(tune_ok_args);
    state_handler->add_encoder<MethodOpenArgsEncoder>(open_args);

    state_ = State::kWaitingTune;
  } else if (state_ == State::kWaitingTune) {
    state_handler = CreateStateHandler();
    state_handler->set_decoder<MethodOpenOkArgsDecoder>();
    state_ = State::kWaitingOpenOk;
  } else {
    CHECK(state_ == State::kWaitingOpenOk) << "Invalid state: " << state_;
    state_ = State::kDone;
    ;
  }
  return std::move(state_handler);
}
}
