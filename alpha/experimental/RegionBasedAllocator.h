/*
 * =============================================================================
 *
 *       Filename:  RegionBasedAllocator.h
 *        Created:  07/08/15 14:07:45
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __ALPHA_REGION_BASED_ALLOCATOR_H__
#define __ALPHA_REGION_BASED_ALLOCATOR_H__

#include <memory>
#include <type_traits>

namespace alpha {

template <typename T, class Enable = void>
class RegionBasedAllocator;

template <typename T>
class RegionBasedAllocator<
    T,
    typename std::enable_if<std::is_pod<T>::value &&
                            !std::is_pointer<T>::value>::type> {
 public:
  using NodeID = uint32_t;
  static const NodeID kInvalidNodeID;
  struct Header {
    uint64_t magic;
    uint32_t size;
    uint32_t element_size;
    NodeID free;
    NodeID free_list;
  };
  static std::unique_ptr<RegionBasedAllocator> Create(char* data,
                                                      size_t size,
                                                      Header* header = nullptr);
  static std::unique_ptr<RegionBasedAllocator> Restore(
      char* data, size_t size, Header* header = nullptr);
  NodeID Allocate();
  void Deallocate(NodeID index);
  void clear();
  T* Get(NodeID index);
  const T* Get(NodeID index) const;

  bool empty() const;
  uint32_t size() const;
  uint32_t max_size() const;

 private:
  static const uint64_t kMagic = 0x434da4cca9fea893;
  RegionBasedAllocator();
  NodeID* NodeIDToNodeIDPtr(NodeID index);
  Header* header_;
  T* start_;
  NodeID max_node_id_;
};
}

#include "RegionBasedAllocator-inl.h"

#endif /* ----- #ifndef __ALPHA_REGION_BASED_ALLOCATOR_H__  ----- */
