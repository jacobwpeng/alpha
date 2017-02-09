/*
 * =====================================================================================
 *
 *       Filename:  event_loop.h
 *        Created:  08/22/14 14:34:28
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =====================================================================================
 */

#pragma once

#include <stdint.h>
#include <signal.h>
#include <map>
#include <vector>
#include <memory>
#include <functional>
#include <alpha/compiler.h>
#include <alpha/TimeUtil.h>
#include <alpha/timer_manager.h>

namespace alpha {
class Poller;
class Channel;

class EventLoop {
 public:
  enum ServerStatus { kIdle = 0, kBusy = 1 };
  using CronFunctor = std::function<int(uint64_t)>;
  using Functor = std::function<void(void)>;

 public:
  EventLoop();
  ~EventLoop();
  DISABLE_COPY_ASSIGNMENT(EventLoop);

  void Run();
  int RunForever();
  void Quit();
  void QueueInLoop(const Functor& functor);
  void UpdateChannel(Channel* channel);
  void RemoveChannel(Channel* channel);

  template <typename F, typename... Args>
  typename std::enable_if<sizeof...(Args) != 0, TimerManager::TimerId>::type
  RunAt(alpha::TimeStamp ts, F&& f, Args&&... args);
  template <typename F, typename... Args>
  typename std::enable_if<sizeof...(Args) != 0, TimerManager::TimerId>::type
  RunAfter(uint32_t milliseconds, F&& f, Args&&... args);
  template <typename F, typename... Args>
  typename std::enable_if<sizeof...(Args) != 0, TimerManager::TimerId>::type
  RunEvery(uint32_t milliseconds, F&& f, Args&&... args);
  TimerManager::TimerId RunAt(alpha::TimeStamp ts, const Functor& f);
  TimerManager::TimerId RunAfter(uint32_t milliseconds, const Functor& f);
  TimerManager::TimerId RunEvery(uint32_t milliseconds, const Functor& f);
  void RemoveTimer(TimerManager::TimerId);
  bool Expired(TimerManager::TimerId) const;
  bool TrapSignal(int signal, const Functor& cb);

  void set_busy_timeout(int time) { busy_timeout_ms_ = time; }
  void set_idle_timeout(int idle_time) { idle_timeout_ms_ = idle_time; }
  void set_next_max_timeout(int timeoutms);
  void set_cron_functor(const CronFunctor& functor) { cron_functor_ = functor; }

 private:
  std::unique_ptr<Poller> poller_;
  bool quit_;
  uint64_t iteration_;
  int busy_timeout_ms_;
  int idle_timeout_ms_;
  int next_timeout_ms_;
  CronFunctor cron_functor_;
  std::unique_ptr<TimerManager> timer_manager_;
  std::vector<Functor> queued_functors_;
  std::map<int, Functor> signal_handlers_;
};
}

#include <alpha/event_loop-inl.h>
