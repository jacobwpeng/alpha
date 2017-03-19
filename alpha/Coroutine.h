/*
 * ==============================================================================
 *
 *       Filename:  Coroutine.h
 *        Created:  03/29/15 12:28:13
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#pragma once

#include <ucontext.h>
#include <cstdint>
#include <vector>
#include <atomic>

namespace alpha {
class Coroutine {
 public:
  enum class Status { kReady = 0, kSuspended = 1, kRunning = 2, kDead = 3 };
  static const size_t kMaxStackSize = 1 << 20;

  Coroutine();
  virtual ~Coroutine();
  virtual void Routine() = 0;

  void Yield();
  virtual void Resume();
  Status status() const;
  bool IsRunning() const;
  bool IsDead() const;
  bool IsSuspended() const;
  int64_t id() const { return id_; }

 private:
  static void InternalRoutine(uint32_t hi, uint32_t lo);
  void Done();

 private:
  void SaveStack();
  void RestoreStack();
  static std::atomic_int next_coroutine_id_;
  Status status_;
  ucontext_t yield_recovery_point_;
  ucontext_t execution_point_;
  size_t real_stack_size_;
  int64_t id_;
  std::vector<char> stack_;
};
}
