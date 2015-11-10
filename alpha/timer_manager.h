/*
 * =============================================================================
 *
 *       Filename:  timer_manager.h
 *        Created:  04/05/15 17:07:56
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __TIMER_MANAGER_H__
#define __TIMER_MANAGER_H__

#include "time_util.h"
#include <vector>
#include <memory>
#include <functional>

namespace alpha {
class TimerManager {
 public:
  using TimerId = uint64_t;
  using TimerFunctor = std::function<void(void)>;
  using TimerFunctorList = std::vector<TimerFunctor>;

  TimerManager();
  ~TimerManager();
  TimerId AddTimer(alpha::TimeStamp expire_time, TimerFunctor functor);
  TimerId AddPeriodicalTimer(alpha::TimeStamp expire_time, uint32_t interval_ms,
                             TimerFunctor functor);
  void RemoveTimer(TimerId timer);
  TimerFunctorList Step(alpha::TimeStamp now);
  bool Expired(TimerId timer) const;

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};
}

#endif /* ----- #ifndef __TIMER_MANAGER_H__  ----- */
