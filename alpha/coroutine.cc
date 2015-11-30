/*
 * ==============================================================================
 *
 *       Filename:  coroutine.cc
 *        Created:  03/29/15 12:36:29
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#include "coroutine.h"
#include <cstring>
#include "logger.h"

namespace alpha {
thread_local Coroutine* current = nullptr;
thread_local char shared_stack[Coroutine::kMaxStackSize];

Coroutine::Coroutine() : status_(Status::kReady), real_stack_size_(0) {}

Coroutine::~Coroutine() {}

void Coroutine::Yield() {
  SaveStack();
  status_ = Status::kSuspended;
  current = nullptr;
  swapcontext(&execution_point_, &yield_recovery_point_);
}

void Coroutine::Resume() {
  CHECK(current == nullptr) << "Recursive resume called";
  int err = 0;
  auto ptr = reinterpret_cast<uintptr_t>(this);
  switch (status_) {
    case Status::kReady:
      err = getcontext(&execution_point_);
      CHECK(err != -1) << "getcontext failed";
      execution_point_.uc_link = &yield_recovery_point_;
      execution_point_.uc_stack.ss_sp = shared_stack;
      execution_point_.uc_stack.ss_size = sizeof(shared_stack);
      makecontext(&execution_point_, (void (*)(void))Coroutine::InternalRoutine,
                  2, (uint32_t)(ptr >> 32), (uint32_t)(ptr));
      status_ = Status::kRunning;
      current = this;
      swapcontext(&yield_recovery_point_, &execution_point_);
      break;
    case Status::kSuspended:
      RestoreStack();
      status_ = Status::kRunning;
      current = this;
      swapcontext(&yield_recovery_point_, &execution_point_);
      break;
    default:
      CHECK(false) << "Invalid status: " << status_;
      break;
  }
}

Coroutine::Status Coroutine::status() const { return status_; }

bool Coroutine::IsRunning() const { return status_ == Status::kRunning; }

bool Coroutine::IsSuspended() const { return status_ == Status::kSuspended; }

bool Coroutine::IsDead() const { return status_ == Status::kDead; }

void Coroutine::Done() { status_ = Status::kDead; }

void Coroutine::SaveStack() {
  char dummy = 0;
  char* addr = &dummy;
  CHECK(IsRunning()) << "Coroutine is not running, status: " << status();
  CHECK(addr >= shared_stack) << "Invalid addr: " << addr;
  CHECK(addr <= Coroutine::kMaxStackSize + shared_stack)
      << "Invalid addr: " << addr;
  size_t real_stack_size = shared_stack + Coroutine::kMaxStackSize - addr;
  if (stack_.size() < real_stack_size) {
    stack_.resize(real_stack_size);
  } else if (stack_.size() > real_stack_size * 2) {
    stack_.resize(real_stack_size);
  }

  real_stack_size_ = real_stack_size;
  memcpy(stack_.data(), addr, real_stack_size);
}

void Coroutine::RestoreStack() {
  CHECK(real_stack_size_ <= sizeof(shared_stack))
      << "Invalid real_stack_size_: " << real_stack_size_;
  char* addr = shared_stack + Coroutine::kMaxStackSize - real_stack_size_;
  memcpy(addr, stack_.data(), real_stack_size_);
}

void Coroutine::InternalRoutine(uint32_t hi, uint32_t lo) {
  uintptr_t ptr =
      (static_cast<uintptr_t>(hi) << 32) | static_cast<uintptr_t>(lo);
  auto co = reinterpret_cast<Coroutine*>(ptr);
  co->Routine();
  co->Done();
  current = nullptr;
}
}
