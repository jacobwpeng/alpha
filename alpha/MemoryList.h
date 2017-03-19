/*
 * ==============================================================================
 *
 *       Filename:  MemoryList.h
 *        Created:  04/26/15 15:14:23
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#ifndef __MEMORY_LIST_H__
#define __MEMORY_LIST_H__

#include <cstring>
#include <cassert>
#include <cstddef>
#include <memory>
#include <limits>
#include <type_traits>
#include <alpha/Compiler.h>
#include <alpha/Logger.h>

namespace alpha {
template <typename T, typename Enable = void>
class MemoryList;

template <typename T>
class MemoryList<T,
                 typename std::enable_if<std::is_pod<T>::value &&
                                         !std::is_pointer<T>::value>::type> {
 public:
  using SizeType = uint32_t;
  using NodeId = uint32_t;
  static const NodeId kInvalidNodeId = std::numeric_limits<NodeId>::max();
  static std::unique_ptr<MemoryList> Create(char* buffer, SizeType size);
  static std::unique_ptr<MemoryList> Restore(char* buffer, SizeType size);

  NodeId Allocate();
  void Deallocate(NodeId id);
  void Clear();
  T* Get(NodeId);
  const T* Get(NodeId) const;
  SizeType size() const;
  SizeType max_size() const;
  bool empty() const;

 private:
  struct Header {
    int64_t magic;
    SizeType size;
    SizeType buffer_size;
    SizeType node_size;
    NodeId free_list;
    NodeId free_area;
  };
  static const int64_t kMagic = 0x434da4cca9fea893;

  NodeId* NodeIdToNodeIdPtr(NodeId id);
  char* NodeIdToAddress(NodeId id);

  NodeId base_;
  Header* header_;
  const char* buffer_;
};

#include <alpha/MemoryList-inl.h>
}

#endif /* ----- #ifndef __MEMORY_LIST_H__  ----- */
