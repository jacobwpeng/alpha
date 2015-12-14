/*
 * ==============================================================================
 *
 *       Filename:  slice.h
 *        Created:  12/24/14 11:07:57
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#pragma once

#include <cstddef>
#include <cassert>
#include <string>
#include <type_traits>

namespace alpha {

class Slice {
 public:
  static const size_t npos = (size_t)(-1);

 public:
  Slice();
  Slice(const char* s, size_t len);
  Slice(const char* s);
  Slice(const std::string& str);
  Slice(const Slice& slice);

  template <typename T>
  Slice(const T* t, typename std::enable_if<
                        std::is_pod<T>::value && !std::is_pointer<T>::value,
                        void*>::type dummy = nullptr)
      : buf_(reinterpret_cast<const char*>(t)), len_(sizeof(T)) {
    (void)dummy;
  }

  template <typename T>
  typename std::enable_if<std::is_pod<T>::value && !std::is_pointer<T>::value,
                          const T*>::type
  as() const {
    assert(sizeof(T) <= len_);
    return reinterpret_cast<const T*>(buf_);
  }

  Slice subslice(size_t pos = 0, size_t len = npos);

  void Clear();
  bool empty() const { return buf_ == NULL || len_ == 0; }
  const char* data() const { return buf_; }
  const char* begin() const { return buf_; }
  const char* end() const { return empty() ? buf_ : buf_ + len_; };
  size_t size() const { return len_; }
  /* KMP style find using next array */
  size_t find(const Slice& s) const;
  bool StartsWith(Slice prefix) const;
  bool EndsWith(Slice suffix) const;
  bool RemovePrefix(Slice prefix);
  bool RemoveSuffix(Slice suffix);
  void Advance(size_t n);
  void Subtract(size_t n);

  std::string ToString() const;

 private:
  /* KMP style find using DFA */
  size_t InternalFind(const char* p) const;

 private:
  const char* buf_;
  size_t len_;
};

bool operator<(const Slice& lhs, const Slice& rhs);
bool operator==(const Slice& lhs, const Slice& rhs);
}
