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
#include <cassert>
#include <cstring>
#include "logger.h"

namespace alpha {
    __thread char shared_stack[Coroutine::kMaxStackSize];

    Coroutine::Coroutine()
        :status_(Status::kSuspended), real_stack_size_(0) {
    }

    Coroutine::~Coroutine() {
    }

    void Coroutine::Yield() {
        char dummy;
        char* addr = &dummy;
        assert (addr >= shared_stack);
        assert (addr <= Coroutine::kMaxStackSize + shared_stack);
        size_t real_stack_size = shared_stack + Coroutine::kMaxStackSize - addr;
        if (stack_.size() < real_stack_size) {
            stack_.resize(real_stack_size);
        } else if (stack_.size() > real_stack_size * 2) {
            stack_.resize(real_stack_size);
        }

        real_stack_size_ = real_stack_size;
        memcpy(stack_.data(), addr, real_stack_size);
        status_ = Status::kSuspended;
        swapcontext(&execution_point_, &yield_recovery_point_);
    }

    void Coroutine::Resume() {
        assert (IsSuspended());
        if (stack_.empty()) {
            if (IsDead()) {
                /* Do cleanup */
            } else {
                int err = getcontext(&execution_point_);
                assert (err != -1);
                (void)err;
                execution_point_.uc_link = &yield_recovery_point_;
                execution_point_.uc_stack.ss_sp = shared_stack;
                execution_point_.uc_stack.ss_size = sizeof(shared_stack);
                auto routine = (void (*)(void)) Coroutine::InternalRoutine;
                makecontext(&execution_point_, routine, 1, this);
                status_ = Status::kRunning;
                swapcontext(&yield_recovery_point_, &execution_point_);
            }
        } else {
            assert (real_stack_size_ <= sizeof(shared_stack));
            char* addr = shared_stack + Coroutine::kMaxStackSize - real_stack_size_;
            memcpy(addr, stack_.data(), real_stack_size_);
            swapcontext(&yield_recovery_point_, &execution_point_);
        }
    }

    Coroutine::Status Coroutine::status() const {
        return status_;
    }

    bool Coroutine::IsRunning() const {
        return status_ == Status::kRunning;
    }

    bool Coroutine::IsSuspended() const {
        return status_ == Status::kSuspended;
    }

    bool Coroutine::IsDead() const {
        return status_ == Status::kDead;
    }

    void Coroutine::Done() {
        status_ = Status::kDead;
    }

    void Coroutine::InternalRoutine(Coroutine* co) {
        co->Routine();
        co->Done();
    }
}
