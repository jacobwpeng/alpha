/*
 * =============================================================================
 *
 *       Filename:  ConnectionWorkingFSM.cc
 *        Created:  11/22/15 16:40:23
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "ConnectionWorkingFSM.h"

namespace amqp {
ConnectionWorkingFSM::ConnectionWorkingFSM(const ConnectionPtr& conn)
    : conn_(conn) {}

FSM::Status ConnectionWorkingFSM::HandleFrame(FramePtr&& frame) {
  return Status::kDone;
  // bool should_close = conn_->HandleFrame(std::move(frame));
  // return should_close ? Status::kDone : Status::kWaitMoreFrame;
}

bool ConnectionWorkingFSM::FlushReply() { return true; }

bool ConnectionWorkingFSM::WriteInitialRequest() { return true; }
}
