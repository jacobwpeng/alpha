/*
 * =============================================================================
 *
 *       Filename:  AsyncTcpConnectionCoroutine.cc
 *        Created:  12/01/15 11:40:21
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "AsyncTcpConnectionCoroutine.h"
#include "AsyncTcpClient.h"
#include "event_loop.h"
#include "logger.h"

namespace alpha {
AsyncTcpConnectionCoroutine::AsyncTcpConnectionCoroutine(
    AsyncTcpClient* owner, const CoroutineFunc& func)
    : timeout_(false), owner_(owner), func_(func), timeout_timer_id_(0) {}

AsyncTcpConnectionCoroutine::~AsyncTcpConnectionCoroutine() {
  MaybeCancelTimeoutTimer();
  DLOG_INFO << "AsyncTcpConnectionCoroutine destroyed, id: " << id();
}

void AsyncTcpConnectionCoroutine::Routine() {
  func_(owner_, this);
  auto co = this;
  auto owner = owner_;
  owner_->loop()->QueueInLoop([owner, co] { owner->RemoveCoroutine(co); });
}

void AsyncTcpConnectionCoroutine::Resume() {
  MaybeCancelTimeoutTimer();
  Coroutine::Resume();
}

void AsyncTcpConnectionCoroutine::YieldWithTimeout(int timeout) {
  timeout_ = false;
  timeout_timer_id_ = owner_->loop()->RunAfter(timeout, [this] {
    timeout_timer_id_ = 0;
    if (this->IsSuspended()) {
      this->timeout_ = true;
      this->Resume();
    }
  });
  Yield();
}

void AsyncTcpConnectionCoroutine::MaybeCancelTimeoutTimer() {
  if (timeout_timer_id_) {
    owner_->loop()->RemoveTimer(timeout_timer_id_);
    timeout_timer_id_ = 0;
  }
}
}
