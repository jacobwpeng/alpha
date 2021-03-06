/*
 * ==============================================================================
 *
 *       Filename:  TimeUtil.cc
 *        Created:  03/29/15 17:52:29
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#include <alpha/TimeUtil.h>
#include <chrono>

namespace alpha {
TimeStamp Now() {
  auto now = std::chrono::system_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             now.time_since_epoch())
      .count();
}

TimeStamp NowInSeconds() { return Now() / kMilliSecondsPerSecond; }

TimeStamp from_time_t(time_t t) {
  return static_cast<TimeStamp>(t) * kMilliSecondsPerSecond;
}

time_t to_time_t(TimeStamp ts) { return ts / kMilliSecondsPerSecond; }

bool InSameHour(TimeStamp lhs, TimeStamp rhs) {
  return (lhs % kSecondsPerHour) == (rhs % kSecondsPerHour);
}
}
