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

namespace alpha {
AsyncTcpConnectionCoroutine::AsyncTcpConnectionCoroutine(
    AsyncTcpClient* owner, const CoroutineFunc& func)
    : owner_(owner), func_(func) {}

void AsyncTcpConnectionCoroutine::Routine() {
  func_(owner_, this);
  auto co = this;
  auto owner = owner_;
  owner_->loop()->QueueInLoop([owner, co] { owner->RemoveCoroutine(co); });
}
}
