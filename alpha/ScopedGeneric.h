/*
 * =============================================================================
 *
 *       Filename:  ScopedGeneric.h
 *        Created:  01/08/16 23:42:07
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once
#include <unistd.h>
#include <alpha/Logger.h>

namespace alpha {
template <typename T, typename Traits>
class ScopedGeneric {
 public:
  ScopedGeneric() : data_(Traits::InvalidValue()) {}

  explicit ScopedGeneric(T data) : data_(data) {}

  ScopedGeneric(ScopedGeneric&& other) : data_(other.Release()) {}

  ScopedGeneric& operator=(ScopedGeneric&& other) {
    Reset(other.Release());
    return *this;
  }

  ScopedGeneric(const ScopedGeneric& other) = delete;
  ScopedGeneric& operator=(const ScopedGeneric& other) = delete;

  template <typename U, typename TraitsU>
  ScopedGeneric(const ScopedGeneric<U, TraitsU>& other) = delete;
  template <typename U, typename TraitsU>
  ScopedGeneric& operator=(const ScopedGeneric<U, TraitsU>& other) = delete;

  template <typename U, typename TraitsU>
  ScopedGeneric(ScopedGeneric<U, TraitsU>&& other) = delete;
  template <typename U, typename TraitsU>
  ScopedGeneric& operator=(ScopedGeneric<U, TraitsU>&& other) = delete;

  ~ScopedGeneric() { MaybeFree(); }

  void Reset(T data = Traits::InvalidValue()) {
    if (data_ == data && data_ != Traits::InvalidValue()) {
      CHECK(false) << "Invalid self-assign";
    }
    MaybeFree();
    data_ = data;
  }

  T Release() {
    T old = data_;
    data_ = Traits::InvalidValue();
    return old;
  }

  const T& get() const { return data_; }

  void swap(ScopedGeneric& other) {
    using std::swap;
    swap(data_, other.data_);
  }

  bool is_valid() const { return data_ != Traits::InvalidValue(); }

  bool operator==(const ScopedGeneric& other) const {
    return data_ == other.data_;
  }
  bool operator!=(const ScopedGeneric& other) const {
    return !(*this == other);
  }

  template <typename U, typename TraitsU>
  bool operator==(const ScopedGeneric<U, TraitsU>& other) const = delete;
  template <typename U, typename TraitsU>
  bool operator!=(const ScopedGeneric<U, TraitsU>& other) const = delete;

 private:
  void MaybeFree() {
    if (data_ != Traits::InvalidValue()) {
      Traits::Free(data_);
      data_ = Traits::InvalidValue();
    }
  }
  T data_;
};

// Some useful typedefs
struct ScopedFDTraits {
  static int InvalidValue() { return -1; }
  static void Free(int fd) { ::close(fd); }
};
using ScopedFD = ScopedGeneric<int, ScopedFDTraits>;

struct ScopedFILETraits {
  static FILE* InvalidValue() { return nullptr; }
  static void Free(FILE* fp) { ::fclose(fp); }
};
using ScopedFILE = ScopedGeneric<FILE*, ScopedFILETraits>;
}
