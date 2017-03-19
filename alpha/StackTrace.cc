/*
 * =============================================================================
 *
 *       Filename:  StackTrace.cc
 *        Created:  12/12/15 23:21:42
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <alpha/StackTrace.h>
#include <execinfo.h>

namespace alpha {
int GetStackTrace(void** result, int max_depth, int skip_count) {
  // glog:stacktrace_general-inl.h
  static const int kStackLength = 64;
  void* stack[kStackLength];
  int size;

  size = backtrace(stack, kStackLength);
  skip_count++;  // we want to skip the current frame as well
  int result_count = size - skip_count;
  if (result_count < 0) result_count = 0;
  if (result_count > max_depth) result_count = max_depth;
  for (int i = 0; i < result_count; i++) result[i] = stack[i + skip_count];

  return result_count;
}
}
