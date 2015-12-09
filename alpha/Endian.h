/*
 * =============================================================================
 *
 *       Filename:  Endian.h
 *        Created:  12/09/15 14:36:09
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __ALPHA_ENDIAN_H__
#define __ALPHA_ENDIAN_H__

#include <byteswap.h>
#include <cassert>
#include <type_traits>

namespace alpha {
template <typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type BSwap(T x) {
  if (sizeof(T) == 1) {
    return x;
  } else if (sizeof(T) == 2) {
    return bswap_16(x);
  } else if (sizeof(T) == 4) {
    return bswap_32(x);
  } else {
    assert(sizeof(T) == 8);
    return bswap_64(x);
  }
}
template <typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type HostToBigEndian(
    T x) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  return BSwap(x);
#else
  return x;
#endif
}

template <typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type BigEndianToHost(
    T x) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  return BSwap(x);
#else
  return x;
#endif
}

template <typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type HostToLittleEndian(
    T x) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  return x;
#else
  return BSwap(x);
#endif
}

template <typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type LittleEndianToHost(
    T x) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  return x;
#else
  return BSwap(x);
#endif
}
}

#endif /* ----- #ifndef __ENDIAN_H__  ----- */
