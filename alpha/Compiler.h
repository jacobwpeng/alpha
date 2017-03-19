/*
 * ==============================================================================
 *       Filename:  Compiler.h
 *        Created:  15:07:46 Dec 23, 2014
 *         Author:  jacobwpeng
 *          Email:  jacobwpeng@tencent.com
 *    Description:  compiler optimization tools
 *
 * ==============================================================================
 */

#pragma once

#ifndef likely
#define likely(x) __builtin_expect((x), 1)
#endif

#ifndef unlikely
#define unlikely(x) __builtin_expect((x), 0)
#endif

#define DISABLE_COPY_ASSIGNMENT(type) \
  type(const type&) = delete;         \
  type& operator=(const type&) = delete

#define DISABLE_MOVE_ASSIGNMENT(type) \
  type(type&&) = delete;              \
  type& operator=(type&&) = delete

#include <memory>

namespace alpha {
template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
}
