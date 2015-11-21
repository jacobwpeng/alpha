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
ConnectionCloseFirstStage::ConnectionCloseFirstStage(CodedWriterBase* w,
                                                     const CodecEnv* codec_env)
    : should_goto_second_stage_(false),
      w_(w),
      e_(MethodCloseOkArgs(), codec_env),
      generic_decoder_(codec_env) {}

bool ConnectionCloseFirstStage::HandleFrame(FramePtr&& frame) {
  if (frame->type() != Frame::Type::kMethod) {
    return false;
  }
  auto ret = generic_decoder_.Decode(std::move(frame));
  if (ret == kError) {
    // TODO: throw a ConnectionException
    CHECK(false);
    return false;
  } else if (ret == kNeedsMore) {
    return false;
  } else {
    auto accurate_decoder = generic_decoder_.accurate_decoder();
    auto class_id = accurate_decoder->class_id();
    auto method_id = accurate_decoder->method_id();
    if (class_id == kClassConnectionID) {
      if (method_id == MethodCloseOkArgs::kMethodID) {
        // Receive Close-OK, we are done here
        should_goto_second_stage_ = false;
        return true;
      } else if (method_id == MethodCloseArgs::kMethodID) {
        // Receive Close, we should send Close-OK
        should_goto_second_stage_ = true;
        return true;
      }
    }
    generic_decoder_.Reset();  // Reset unexpected methods
    return false;
  }
}

bool ConnectionCloseFirstStage::WriteReply() {
  if (should_goto_second_stage()) {
    return e_.Encode(w_);
  } else {
    return true;
  }
}

ConnectionCloseSecondStage::ConnectionCloseSecondStage(
    const CodecEnv* codec_env)
    : d_(codec_env) {}

bool ConnectionCloseSecondStage::HandleFrame(FramePtr&& frame) {
  auto rc = d_.Decode(frame->payload());
  if (rc == DecodeState::kDone) {
    return true;
  } else if (rc == DecodeState::kNeedsMore) {
    return false;
  } else {
    // TODO: throw a ConnectionExpcetion
    CHECK(false);
    return false;
  }
}

bool ConnectionCloseSecondStage::WriteReply() { return true; }

ConnectionCloseFSM::ConnectionCloseFSM(CodedWriterBase* w, const CodecEnv* env,
                                       const MethodCloseArgs& method_close_args)
    : FSM(w, env),
      generic_decoder_(env),
      method_close_args_encoder_(method_close_args, env),
      frame_packer_(0, Frame::Type::kMethod, &method_close_args_encoder_) {
  state_ = State::kFirstStage;
  state_handler_ = alpha::make_unique<ConnectionCloseFirstStage>(w, env);
}

FSM::Status ConnectionCloseFSM::HandleFrame(FramePtr&& frame) {
  auto done = state_handler_->HandleFrame(std::move(frame));
  if (!done) {
    return Status::kWaitMoreFrame;
  }
  done = state_handler_->WriteReply();
  if (!done) {
    return Status::kWaitForWrite;
  }
  if (state_ == State::kFirstStage) {
    auto handle =
        dynamic_cast<ConnectionCloseFirstStage*>(state_handler_.get());
    CHECK(handle);
    if (handle->should_goto_second_stage()) {
      handle = nullptr;
      state_handler_ =
          alpha::make_unique<ConnectionCloseSecondStage>(codec_env_);
      return Status::kWaitMoreFrame;
    } else {
      return Status::kDone;
    }
  } else {
    return Status::kDone;
  }
}

bool ConnectionCloseFSM::FlushReply() { return true; }

bool ConnectionCloseFSM::WriteInitialRequest() {
  CHECK(state_ == State::kFirstStage);
  // Write Method Close
  return frame_packer_.WriteTo(w_);
}
}
