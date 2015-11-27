/*
 * =============================================================================
 *
 *       Filename:  ConnectionCloseFSM.cc
 *        Created:  11/19/15 20:35:49
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "ConnectionCloseFSM.h"

namespace amqp {
std::unique_ptr<ConnectionCloseFSM> ConnectionCloseFSM::ActiveClose(
    const CodecEnv* codec_env, SendReplyFunc send_reply_func,
    const MethodCloseArgs& close_args) {
  auto fsm =
      std::unique_ptr<ConnectionCloseFSM>(new ConnectionCloseFSM(codec_env));
  fsm->state_ = State::kWaitingCloseOk;
  send_reply_func(
      0, Frame::Type::kMethod,
      alpha::make_unique<MethodCloseArgsEncoder>(close_args, codec_env));
  return std::move(fsm);
}

std::unique_ptr<ConnectionCloseFSM> ConnectionCloseFSM::PassiveClose(
    const CodecEnv* codec_env, SendReplyFunc send_reply_func) {
  auto fsm =
      std::unique_ptr<ConnectionCloseFSM>(new ConnectionCloseFSM(codec_env));
  send_reply_func(0, Frame::Type::kMethod,
                  alpha::make_unique<MethodCloseOkArgsEncoder>(
                      MethodCloseOkArgs(), codec_env));
  fsm->state_ = State::kDone;
  return std::move(fsm);
}

bool ConnectionCloseFSM::done() const { return state_ == State::kDone; }

FSM::Status ConnectionCloseFSM::HandleFrame(FramePtr&& frame,
                                            SendReplyFunc send_reply_func) {
  CHECK(!done()) << "Handle more frames when FSM is done";
  // Ignore all frame types except method
  if (frame->type() != Frame::Type::kMethod) {
    return Status::kWaitMoreFrame;
  }

  int rc = generic_decoder_.Decode(std::move(frame));
  if (rc == DecodeState::kNeedsMore) {
    DLOG_INFO << "More frame to proceed";
    return Status::kWaitMoreFrame;
  } else if (rc == DecodeState::kError) {
    CHECK(false);
  } else {
    // Decode done
  }
  auto class_id = generic_decoder_.accurate_decoder()->class_id();
  auto method_id = generic_decoder_.accurate_decoder()->method_id();
  DLOG_INFO << "ClassID: " << class_id << ", MethodID: " << method_id;
  // Ignore all methods except close/close-ok
  if (class_id != kClassConnectionID) {
    return Status::kWaitMoreFrame;
  }

  if (method_id != MethodCloseArgs::kMethodID &&
      method_id != MethodCloseOkArgs::kMethodID) {
    return Status::kWaitMoreFrame;
  }

  if (method_id == MethodCloseOkArgs::kMethodID) {
    state_ = State::kDone;
  } else {
    // Simultaneous Close
    send_reply_func(0, Frame::Type::kMethod,
                    alpha::make_unique<MethodCloseOkArgsEncoder>(
                        MethodCloseOkArgs(), codec_env_));
  }
  return Status::kDone;
}

ConnectionCloseFSM::ConnectionCloseFSM(const CodecEnv* codec_env)
    : FSM(codec_env_), generic_decoder_(codec_env) {}
}
