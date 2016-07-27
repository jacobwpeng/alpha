/*
 * =============================================================================
 *
 *       Filename:  event_loop-inl.h
 *        Created:  06/02/16 10:45:51
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <alpha/logger.h>

namespace alpha {

template <typename F, typename... Args>
typename std::enable_if<sizeof...(Args) != 0, TimerManager::TimerId>::type
EventLoop::RunAt(alpha::TimeStamp ts, F&& f, Args&&... args) {
  return RunAt(ts, std::bind(std::forward<F>(f), std::forward<Args>(args)...));
}

template <typename F, typename... Args>
typename std::enable_if<sizeof...(Args) != 0, TimerManager::TimerId>::type
EventLoop::RunAfter(uint32_t milliseconds, F&& f, Args&&... args) {
  return RunAfter(milliseconds,
                  std::bind(std::forward<F>(f), std::forward<Args>(args)...));
}

template <typename F, typename... Args>
typename std::enable_if<sizeof...(Args) != 0, TimerManager::TimerId>::type
EventLoop::RunEvery(uint32_t milliseconds, F&& f, Args&&... args) {
  return RunEvery(milliseconds,
                  std::bind(std::forward<F>(f), std::forward<Args>(args)...));
}
}
