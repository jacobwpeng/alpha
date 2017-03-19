/*
 * =============================================================================
 *
 *       Filename:  RegionBasedAllocator-inl.h
 *        Created:  07/08/15 14:43:04
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __ALPHA_REGION_BASED_ALLOCATOR_H__
#error This file should only be included from RegionBasedAllocator.h
#endif

#include "RegionBasedHelper.h"
#include <cassert>

#define AllocatorType                                  \
  RegionBasedAllocator<                                \
      T,                                               \
      typename std::enable_if<std::is_pod<T>::value && \
                              !std::is_pointer<T>::value>::type>

namespace alpha {

template <typename T>
const typename AllocatorType::NodeID AllocatorType::kInvalidNodeID = 0;

template <typename T>
std::unique_ptr<AllocatorType> AllocatorType::Create(char* data,
                                                     size_t size,
                                                     Header* header) {
  static_assert(
      alignof(Header) <= kAlignmentRequired && alignof(T) <= kAlignmentRequired,
      "kAlignmentRequired is not enough!");
  if (CheckAligned(data) == false) {
    return nullptr;
  }
  auto p = data;
  if (header == nullptr) {
    header = reinterpret_cast<Header*>(p);
    p += sizeof(Header);
  }
  auto data_start = Align<T>(p);
  size_t min_size = (data_start - data);
  if (size < min_size) {
    return nullptr;
  }
  header->magic = kMagic;
  header->size = 0;
  header->element_size = sizeof(T);
  header->free_list = kInvalidNodeID;
  std::unique_ptr<AllocatorType> alloc(new RegionBasedAllocator());
  alloc->header_ = header;
  alloc->start_ = reinterpret_cast<T*>(data_start);
  alloc->max_node_id_ = (data + size - data_start) / sizeof(T);
  header->free = std::min<NodeID>(alloc->max_node_id_, 1);
  return alloc;
}

template <typename T>
std::unique_ptr<AllocatorType> AllocatorType::Restore(char* data,
                                                      size_t size,
                                                      Header* header) {
  if (CheckAligned(data) == false) {
    return nullptr;
  }
  auto p = data;
  if (header == nullptr) {
    header = reinterpret_cast<Header*>(p);
    p += sizeof(Header);
  }
  auto data_start = Align<T>(p);
  size_t min_size = data_start - data;
  if (size < min_size) {
    return nullptr;
  }
  auto max_node_id = (data + size - data_start) / sizeof(T);
  if (header->magic != kMagic || header->size > max_node_id ||
      header->element_size != sizeof(T) || header->free_list > max_node_id ||
      header->free > max_node_id + 1) {
    return nullptr;
  }
  std::unique_ptr<AllocatorType> alloc(new RegionBasedAllocator());
  alloc->header_ = header;
  alloc->start_ = reinterpret_cast<T*>(data_start);
  alloc->max_node_id_ = max_node_id;
  return alloc;
}

template <typename T>
typename AllocatorType::NodeID AllocatorType::Allocate() {
  NodeID res = kInvalidNodeID;
  if (header_->free_list != kInvalidNodeID) {
    res = header_->free_list;
    header_->free_list = *NodeIDToNodeIDPtr(header_->free_list);
  } else if (header_->free > max_node_id_) {
    throw std::bad_alloc();
  } else {
    res = header_->free;
    ++header_->free;
  }
  ++header_->size;
  return res;
}

template <typename T>
void AllocatorType::Deallocate(NodeID index) {
  assert(index != kInvalidNodeID);
  assert(index <= max_node_id_);
  *NodeIDToNodeIDPtr(index) = header_->free_list;
  header_->free_list = index;
  --header_->size;
}

template <typename T>
void AllocatorType::clear() {
  header_->size = 0;
  header_->free = std::min<NodeID>(max_node_id_, 1);
  header_->free_list = kInvalidNodeID;
}

template <typename T>
T* AllocatorType::Get(NodeID index) {
  assert(index != kInvalidNodeID);
  assert(index <= max_node_id_);
  return start_ + index - 1;  // 1 <= index <= max_node_id_
}

template <typename T>
const T* AllocatorType::Get(NodeID index) const {
  assert(index != kInvalidNodeID);
  assert(index <= max_node_id_);
  return start_ + index - 1;
}

template <typename T>
bool AllocatorType::empty() const {
  return size() == 0;
}

template <typename T>
uint32_t AllocatorType::size() const {
  return header_->size;
}

template <typename T>
uint32_t AllocatorType::max_size() const {
  return max_node_id_;
}

template <typename T>
AllocatorType::RegionBasedAllocator()
    : header_(nullptr), start_(nullptr), max_node_id_(0) {}

template <typename T>
typename AllocatorType::NodeID* AllocatorType::NodeIDToNodeIDPtr(NodeID index) {
  static_assert(sizeof(T) >= sizeof(NodeID), "sizeof(T) < sizeof(NodeID)");
  assert(index != kInvalidNodeID);
  assert(index <= max_node_id_);
  return reinterpret_cast<NodeID*>(Get(index));
}
}

#undef AllocatorType
