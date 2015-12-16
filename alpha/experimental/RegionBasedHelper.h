/*
 * =============================================================================
 *
 *       Filename:  RegionBasedHelper.h
 *        Created:  07/08/15 14:46:42
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __ALPHA_REGION_BASED_HELPER_H__
#define __ALPHA_REGION_BASED_HELPER_H__

static const uint8_t kAlignmentRequired = 16;
static const uint8_t kAlignmentMask = kAlignmentRequired - 1;
static_assert(kAlignmentRequired != 0 &&
                  (kAlignmentRequired & (kAlignmentRequired - 1)) == 0,
              "kAlignmentRequired must be power of 2 and can not be 0");
inline char* Align(char* p) {
  return reinterpret_cast<char*>(
      (reinterpret_cast<uintptr_t>(p) + kAlignmentMask) & ~kAlignmentMask);
}
template <typename T>
inline char* Align(char* p) {
  return reinterpret_cast<char*>(
      (reinterpret_cast<uintptr_t>(p) + alignof(T) - 1) / alignof(T) *
      alignof(T));
}

inline bool CheckAligned(void* p) {
  return (reinterpret_cast<uintptr_t>(p) & kAlignmentMask) == 0;
}

#endif /* ----- #ifndef __ALPHA_REGION_BASED_HELPER_H__  ----- */
