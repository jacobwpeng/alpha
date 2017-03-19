/*
 * =============================================================================
 *
 *       Filename:  RegionBasedVector-inl.h
 *        Created:  07/07/15 14:34:41
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __ALPHA_REGION_BASED_VECTOR_H__
#error This file should only be included from RegionBasedVector.h
#endif

#include "RegionBasedHelper.h"
#include <cassert>
#include <algorithm>

#define VectorType                                                   \
  RegionBasedVector<T,                                               \
                    typename std::enable_if<std::is_pod<T>::value && \
                                            !std::is_pointer<T>::value>::type>

namespace alpha {
template <typename T>
std::unique_ptr<VectorType> VectorType::Create(char* data,
                                               size_t size,
                                               Header* header) {
  if (CheckAligned(data) == false) {
    return nullptr;
  }
  auto p = data;
  if (header == nullptr) {
    auto aligned = Align<Header>(p);
    header = reinterpret_cast<Header*>(aligned);
    p = aligned + kHeaderSize;
  }
  char* aligned_data = Align<T>(p);
  size_t minsize = aligned_data - data;
  if (size < minsize) {
    return nullptr;
  }
  header->magic = VectorType::kMagic;
  header->current = 0;
  header->element_size = sizeof(T);
  size_t capacity = (size - minsize) / sizeof(T);
  return std::unique_ptr<VectorType>(
      new VectorType(header, capacity, reinterpret_cast<T*>(aligned_data)));
}

template <typename T>
std::unique_ptr<VectorType> VectorType::Restore(char* data,
                                                size_t size,
                                                Header* header) {
  if (CheckAligned(data) == false) {
    return nullptr;
  }
  auto p = data;
  if (header == nullptr) {
    auto aligned = Align<Header>(p);
    header = reinterpret_cast<Header*>(aligned);
    p = aligned + kHeaderSize;
  }
  char* aligned_data = Align<T>(p);
  size_t minsize = aligned_data - data;
  if (size < minsize) {
    return nullptr;
  }
  size_t capacity = (size - minsize) / sizeof(T);
  if (header->magic != VectorType::kMagic ||
      header->element_size != sizeof(T) || header->current > capacity) {
    return nullptr;
  }
  return std::unique_ptr<VectorType>(
      new VectorType(header, capacity, reinterpret_cast<T*>(aligned_data)));
}

template <typename T>
T& VectorType::operator[](size_t index) {
  assert(index < size());
  return *(start_ + index);
}

template <typename T>
const T& VectorType::operator[](size_t index) const {
  assert(index < size());
  return *(start_ + index);
}

template <typename T>
T& VectorType::at(size_t index) {
  if (index >= size()) {
    throw std::out_of_range("Index out of range");
  }
  return (*this)[index];
}

template <typename T>
const T& VectorType::at(size_t index) const {
  if (index >= size()) {
    throw std::out_of_range("Index out of range");
  }
  return (*this)[index];
}

template <typename T>
T& VectorType::front() {
  return (*this)[0];
}

template <typename T>
const T& VectorType::front() const {
  return (*this)[0];
}

template <typename T>
T& VectorType::back() {
  assert(size() >= 1);
  return (*this)[size() - 1];
}

template <typename T>
const T& VectorType::back() const {
  assert(size() >= 1);
  return (*this)[size() - 1];
}

template <typename T>
void VectorType::push_back(const T& v) {
  if (size() == max_size()) {
    throw std::bad_alloc();
  }
  start_[header_->current++] = v;
}

template <typename T>
VectorType::RegionBasedVector(Header* header, size_t capacity, T* start)
    : start_(start),
      header_(header),
      capacity_(capacity),
      destroy_func_(nullptr) {}

template <typename T>
size_t VectorType::size() const {
  return header_->current;
}

template <typename T>
size_t VectorType::max_size() const {
  return capacity_;
}

template <typename T>
bool VectorType::empty() const {
  return size() == 0;
}

template <typename T>
bool VectorType::operator==(const RegionBasedVector& rhs) {
  // only check used part
  return std::memcmp(header_, rhs.header_, sizeof(Header)) == 0 &&
         size() == rhs.size() &&
         std::memcmp(begin(), rhs.begin(), size() * sizeof(T)) == 0;
}

template <typename T>
T* VectorType::data() {
  return start_;
}

template <typename T>
const T* VectorType::data() const {
  return start_;
}

template <typename T>
typename VectorType::iterator VectorType::erase(iterator it) {
  if (it < begin() || it >= end()) {
    throw std::out_of_range("Index out of range");
  }
  iterator next = std::next(it);
  T* p = it;
  // check alignment
  if (reinterpret_cast<uintptr_t>(p) & (sizeof(T) - 1)) {
    throw std::runtime_error("Invalid iterator");
  }
  if (destroy_func_) {
    destroy_func_(*it);
  }
  if (next != end()) {
    std::memmove(it, it + 1, std::distance(it + 1, end()) * sizeof(T));
  }
  --header_->current;
  return next;
}

template <typename T>
void VectorType::clear() {
  if (destroy_func_) {
    std::for_each(begin(), end(), [this](T& val) { destroy_func_(val); });
  }
  header_->current = 0;
}

template <typename T>
typename VectorType::DestroyFunc VectorType::on_destroy(
    DestroyFunc destroy_func) {
  auto old = destroy_func_;
  destroy_func_ = destroy_func;
  return old;
}

template <typename T>
typename VectorType::iterator VectorType::begin() {
  return start_;
}

template <typename T>
typename VectorType::iterator VectorType::end() {
  return start_ + header_->current;
}

template <typename T>
typename VectorType::const_iterator VectorType::begin() const {
  return start_;
}

template <typename T>
typename VectorType::const_iterator VectorType::end() const {
  return start_ + header_->current;
}
}
#undef VectorType
