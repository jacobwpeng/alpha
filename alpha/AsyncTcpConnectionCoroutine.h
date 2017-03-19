/*
 * =============================================================================
 *
 *       Filename:  AsyncTcpConnectionCoroutine.h
 *        Created:  12/01/15 11:11:14
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include <alpha/TimerManager.h>
#include <alpha/Coroutine.h>

namespace alpha {
class AsyncTcpClient;
class AsyncTcpConnectionCoroutine final : public Coroutine {
 public:
  using CoroutineFunc =
      std::function<void(AsyncTcpClient*, AsyncTcpConnectionCoroutine*)>;
  AsyncTcpConnectionCoroutine(AsyncTcpClient* owner, const CoroutineFunc& func);
  ~AsyncTcpConnectionCoroutine();
  virtual void Routine() override;
  virtual void Resume() override;
  void YieldWithTimeout(int timeout);
  bool timeout() const { return timeout_; }

 private:
  void MaybeCancelTimeoutTimer();
  bool timeout_;
  AsyncTcpClient* owner_;
  CoroutineFunc func_;
  TimerManager::TimerId timeout_timer_id_;
};
}
