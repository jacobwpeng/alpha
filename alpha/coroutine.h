/*
 * ==============================================================================
 *
 *       Filename:  coroutine.h
 *        Created:  03/29/15 12:28:13
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#ifndef __COROUTINE_H__
#define __COROUTINE_H__

#include <ucontext.h>
#include <vector>

namespace alpha {
class Coroutine {
 public:
  enum class Status { kSuspended = 1, kRunning = 2, kDead = 3 };
  static const size_t kMaxStackSize = 1 << 23;

  Coroutine();
  virtual ~Coroutine();
  virtual void Routine() = 0;

  void Yield();
  void Resume();
  Status status() const;
  bool IsRunning() const;
  bool IsDead() const;
  bool IsSuspended() const;

 private:
  static void InternalRoutine(Coroutine* co);
  void Done();

 private:
  Status status_;
  ucontext_t yield_recovery_point_;
  ucontext_t execution_point_;
  size_t real_stack_size_;
  std::vector<char> stack_;
};
}

#endif /* ----- #ifndef __COROUTINE_H__  ----- */
