/*
 * =============================================================================
 *
 *       Filename:  RegionBasedVector.h
 *        Created:  07/07/15 14:32:49
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */
#ifndef __ALPHA_REGION_BASED_VECTOR_H__
#define __ALPHA_REGION_BASED_VECTOR_H__

#include <cstring>
#include <memory>
#include <type_traits>

template <typename T, class Enable = void>
class RegionBasedVector;

template <typename T>
class RegionBasedVector<
    T, typename std::enable_if<std::is_pod<T>::value &&
                               !std::is_pointer<T>::value>::type> {
 public:
  struct Header {
    uint64_t magic;
    uint32_t current;
    uint32_t element_size;
  };
  using DestroyFunc = void (*)(T&);
  using iterator = T*;
  using const_iterator = const T*;
  static const uint32_t kHeaderSize = sizeof(Header);
  static std::unique_ptr<RegionBasedVector> Create(char* data, size_t size,
                                                   Header* header = nullptr);
  static std::unique_ptr<RegionBasedVector> Restore(char* data, size_t size,
                                                    Header* header = nullptr);
  RegionBasedVector(const RegionBasedVector&) = delete;
  RegionBasedVector& operator=(const RegionBasedVector&) = delete;

  T& operator[](size_t index);
  const T& operator[](size_t index) const;
  T& at(size_t index);
  const T& at(size_t index) const;
  T& front();
  const T& front() const;
  T& back();
  const T& back() const;
  void push_back(const T& v);
  size_t size() const;
  size_t max_size() const;
  bool empty() const;
  bool operator==(const RegionBasedVector& rhs);
  T* data();
  const T* data() const;
  iterator erase(iterator it);
  void clear();
  DestroyFunc on_destroy(DestroyFunc destroy_func);
  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;

 private:
  static const uint64_t kMagic = 0x74548fc3e42b937b;
  RegionBasedVector(Header* header, size_t capacity, T* start);
  T* start_;
  Header* header_;
  size_t capacity_;
  DestroyFunc destroy_func_;
};

#include "RegionBasedVector-inl.h"

#endif /* ----- #ifndef __ALPHA_REGION_BASED_VECTOR_H__  ----- */
