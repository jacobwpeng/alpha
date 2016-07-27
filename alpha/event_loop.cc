/*
 * =====================================================================================
 *
 *       Filename:  event_loop.cc
 *        Created:  08/22/14 15:05:44
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =====================================================================================
 */

#include "event_loop.h"

#include <cassert>
#include <type_traits>
#include <algorithm>
#include "logger.h"
#include "channel.h"
#include "poller.h"
#include "logger.h"

namespace alpha {
static_assert(
    std::is_same<EventLoop::Functor, TimerManager::TimerFunctor>::value,
    "Mismatch functor type");
__thread sig_atomic_t signal_num = 0;

static void signal_handler(int signo) { signal_num = signo; }

EventLoop::EventLoop()
    : quit_(false), iteration_(0), wait_time_(20), idle_time_(100) {
  poller_.reset(new Poller);
  timer_manager_.reset(new TimerManager);
}

EventLoop::~EventLoop() = default;

void EventLoop::Run() {
  //先注册信号处理函数
  ChannelList channels;

  static const int kMaxLoopBeforeIdle = 100;
  int timeout = wait_time_;  // ms
  unsigned idle = 0;

  int status = kIdle;
  while (likely(not quit_)) {
    alpha::TimeStamp now = poller_->Poll(timeout, &channels);
    //先处理信号
    if (unlikely(signal_num != 0)) {
      auto it = signal_handlers_.find(signal_num);
      if (it != signal_handlers_.end()) {
        it->second();
      }
      //看看是不是信号处理函数让咱们退出了
      if (quit_) {
        break;
      }
    }
    ++iteration_;
    DLOG_INFO_IF(!channels.empty()) << "channels.size() = " << channels.size()
                                    << ", now = " << now;
    std::vector<Functor> queued_functors;
    std::swap(queued_functors, queued_functors_);

    //再处理网络消息
    std::for_each(channels.begin(), channels.end(),
                  [](Channel* channel) { channel->HandleEvents(); });
    idle = channels.empty() ? idle + 1 : 0;
    channels.clear();

    //然后是周期函数
    if (cron_functor_) {
      status = cron_functor_(iteration_);
    }

    //再处理延时调用函数
    std::for_each(queued_functors.begin(), queued_functors.end(),
                  [](const Functor& f) { f(); });

    auto timer_functors = timer_manager_->Step(now);
    //最后处理定时器
    std::for_each(timer_functors.begin(), timer_functors.end(),
                  [](const Functor& f) { f(); });

    timeout = wait_time_;
    if (idle >= kMaxLoopBeforeIdle && status == kIdle) {
      timeout = idle_time_;
    }
  }
  LOG_INFO << "EventLoop exiting...";
}

int EventLoop::RunForever() {
  Run();
  return 0;
}

void EventLoop::Quit() { quit_ = true; }

void EventLoop::UpdateChannel(Channel* channel) {
  poller_->UpdateChannel(channel);
}

void EventLoop::RemoveChannel(Channel* channel) {
  poller_->RemoveChannel(channel);
}

TimerManager::TimerId EventLoop::RunAt(alpha::TimeStamp ts, const Functor& f) {
  return timer_manager_->AddTimer(ts, f);
}

TimerManager::TimerId EventLoop::RunAfter(uint32_t milliseconds,
                                          const Functor& f) {
  alpha::TimeStamp expire_time = alpha::Now() + milliseconds;
  return RunAt(expire_time, f);
}

TimerManager::TimerId EventLoop::RunEvery(uint32_t milliseconds,
                                          const Functor& f) {
  alpha::TimeStamp expire_time = alpha::Now() + milliseconds;
  return timer_manager_->AddPeriodicalTimer(expire_time, milliseconds, f);
}

void EventLoop::RemoveTimer(TimerManager::TimerId id) {
  timer_manager_->RemoveTimer(id);
}

bool EventLoop::Expired(TimerManager::TimerId id) const {
  return timer_manager_->Expired(id);
}

bool EventLoop::TrapSignal(int signal, const Functor& cb) {
  //只能在Run之前调用
  assert(iteration_ == 0);
  auto it = signal_handlers_.find(signal);
  if (it == signal_handlers_.end()) {
    auto ret = ::signal(signal, signal_handler);
    if (ret == SIG_ERR) {
      PLOG_WARNING << "TrapSignal failed, signal = " << signal;
      return false;
    }
  }
  signal_handlers_[signal] = cb;
  return true;
}

void EventLoop::QueueInLoop(const EventLoop::Functor& functor) {
  queued_functors_.push_back(functor);
}
}
