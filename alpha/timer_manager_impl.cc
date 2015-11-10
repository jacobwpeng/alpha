/*
 * =============================================================================
 *
 *       Filename:  timer_manager_impl.cc
 *        Created:  04/05/15 17:33:29
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "timer_manager.h"
#include <cassert>
#include <map>
#include <set>
#include <limits>
#include <algorithm>
#include "logger.h"

namespace alpha {
class TimerManager::Impl {
 public:
  Impl() = default;
  ~Impl() = default;

  TimerId AddTimer(alpha::TimeStamp expire_time, TimerFunctor functor);
  TimerId AddPeriodicalTimer(alpha::TimeStamp expire_time, uint32_t interval_ms,
                             TimerFunctor functor);
  void RemoveTimer(TimerId timer);
  TimerFunctorList Step(alpha::TimeStamp now);
  bool Expired(TimerId timer) const;

 private:
  struct Timer {
   public:
    Timer(TimerId id, alpha::TimeStamp expire_time, TimerFunctor functor);
    void SetPeridical(uint32_t interval_ms);
    bool IsPeriodical() const;
    void UpdateExpireTime();

   private:
    friend class Impl;
    TimerId id_;
    alpha::TimeStamp expire_time_;
    uint32_t interval_ms_;
    TimerFunctor functor_;
  };
  using TimerPtr = std::unique_ptr<Timer>;
  using TimerEntry = std::pair<alpha::TimeStamp, Timer*>;

  struct TimerEntryCompare {
    bool operator()(const TimerEntry& lhs, const TimerEntry& rhs);
  };
  using TimerSet = std::set<TimerEntry, TimerEntryCompare>;
  using ActiveTimerMap = std::map<TimerId, TimerPtr>;

  Timer* Insert(alpha::TimeStamp expire_time, TimerFunctor functor);

  TimerId current_;
  TimerSet timers_;
  ActiveTimerMap active_timers_;
};

TimerManager::Impl::Timer::Timer(TimerManager::TimerId id,
                                 TimeStamp expire_time,
                                 TimerManager::TimerFunctor functor)
    : id_(id), expire_time_(expire_time), interval_ms_(0), functor_(functor) {}

void TimerManager::Impl::Timer::SetPeridical(uint32_t interval_ms) {
  interval_ms_ = interval_ms;
}

bool TimerManager::Impl::Timer::IsPeriodical() const {
  return interval_ms_ != 0;
}

void TimerManager::Impl::Timer::UpdateExpireTime() {
  assert(IsPeriodical());
  expire_time_ += interval_ms_;
}

bool TimerManager::Impl::TimerEntryCompare::operator()(const TimerEntry& lhs,
                                                       const TimerEntry& rhs) {
  if (lhs.first == rhs.first) {
    return reinterpret_cast<uintptr_t>(lhs.second) <
           reinterpret_cast<uintptr_t>(rhs.second);
  } else {
    return lhs.first < rhs.first;
  }
}

TimerManager::TimerId TimerManager::Impl::AddTimer(alpha::TimeStamp expire_time,
                                                   TimerFunctor functor) {
  return Insert(expire_time, functor)->id_;
}

TimerManager::TimerId TimerManager::Impl::AddPeriodicalTimer(
    alpha::TimeStamp expire_time, uint32_t interval_ms, TimerFunctor f) {
  Timer* timer = Insert(expire_time, f);
  timer->SetPeridical(interval_ms);
  return timer->id_;
}

void TimerManager::Impl::RemoveTimer(TimerId id) {
  auto active = active_timers_.find(id);
  if (active == active_timers_.end()) {
    //已经触发了
    return;
  }
  TimerEntry entry =
      std::make_pair(active->second->expire_time_, active->second.get());
  auto it = timers_.find(entry);
  assert(it != timers_.end());
  timers_.erase(it);
  active_timers_.erase(active);
}

TimerManager::TimerFunctorList TimerManager::Impl::Step(alpha::TimeStamp now) {
  TimerFunctorList functors;
  Timer* max_timer_ptr =
      reinterpret_cast<Timer*>(std::numeric_limits<uintptr_t>::max());
  TimerEntry max_entry = std::make_pair(now, max_timer_ptr);
  auto pend = timers_.upper_bound(max_entry);
  TimerSet periodical_timers;
  std::transform(timers_.begin(), pend, std::back_inserter(functors),
                 [this, &periodical_timers](const TimerEntry& entry) {
    TimerFunctor f = entry.second->functor_;
    if (entry.second->IsPeriodical()) {
      TimerEntry e = entry;
      e.second->UpdateExpireTime();
      e.first = e.second->expire_time_;
      periodical_timers.insert(e);
    } else {
      active_timers_.erase(entry.second->id_);
    }
    return f;
  });
  timers_.erase(timers_.begin(), pend);
  timers_.insert(periodical_timers.begin(), periodical_timers.end());
  return std::move(functors);
}

TimerManager::Impl::Timer* TimerManager::Impl::Insert(
    alpha::TimeStamp expire_time, TimerFunctor f) {
  TimerId id = ++current_;
  TimerPtr timer(new Timer(id, expire_time, f));
  Timer* p = timer.get();
  TimerEntry entry = std::make_pair(expire_time, timer.get());
  timers_.emplace(entry);
  active_timers_.emplace(id, std::move(timer));
  return p;
}

TimerManager::TimerManager() : impl_(new Impl) {}

TimerManager::~TimerManager() = default;

bool TimerManager::Impl::Expired(TimerId id) const {
  return active_timers_.find(id) == active_timers_.end();
}

TimerManager::TimerId TimerManager::AddTimer(alpha::TimeStamp ts,
                                             TimerFunctor functor) {
  return impl_->AddTimer(ts, functor);
}

TimerManager::TimerId TimerManager::AddPeriodicalTimer(
    alpha::TimeStamp expire_time, uint32_t interval_ms, TimerFunctor f) {
  return impl_->AddPeriodicalTimer(expire_time, interval_ms, f);
}

void TimerManager::RemoveTimer(TimerId id) { return impl_->RemoveTimer(id); }

TimerManager::TimerFunctorList TimerManager::Step(alpha::TimeStamp now) {
  return std::move(impl_->Step(now));
}

bool TimerManager::Expired(TimerId id) const { return impl_->Expired(id); }
}
