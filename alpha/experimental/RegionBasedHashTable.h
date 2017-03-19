/*
 * =============================================================================
 *
 *       Filename:  RegionBasedHashTable.h
 *        Created:  07/07/15 14:52:54
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */
#ifndef __ALPHA_REGION_BASED_HASH_TABLE_H__
#define __ALPHA_REGION_BASED_HASH_TABLE_H__

#include <type_traits>
#include "RegionBasedVector.h"
#include "RegionBasedAllocator.h"

namespace alpha {

using NodeID = uint32_t;
template <typename Key,
          typename T,
          typename Hash,
          typename Pred,
          typename KeyOfValue,
          class Enable = void>
class RegionBasedHashTable;

template <typename T>
struct HashTableNode {
  NodeID next;
  T val;
};

template <typename Key,
          typename T,
          typename Hash,
          typename Pred,
          typename KeyOfValue>
class HashTableConstIterator;

template <typename Key,
          typename T,
          typename Hash,
          typename Pred,
          typename KeyOfValue>
class HashTableIterator : public std::iterator<std::forward_iterator_tag, T> {
 public:
  using _HashTable = RegionBasedHashTable<Key, T, Hash, Pred, KeyOfValue>;
  using _HashTableNode = HashTableNode<T>;
  using _Base = std::iterator<std::forward_iterator_tag, T>;

  HashTableIterator();
  HashTableIterator(_HashTable* ht, _HashTableNode* node);
  HashTableIterator& operator++();
  HashTableIterator operator++(int);
  typename _Base::reference operator*() const;
  typename _Base::pointer operator->() const;
  bool operator==(const HashTableIterator& rhs) const;
  bool operator!=(const HashTableIterator& rhs) const;

 private:
  friend class HashTableConstIterator<Key, T, Hash, Pred, KeyOfValue>;
  _HashTable* ht_;
  _HashTableNode* node_;
};

template <typename Key,
          typename T,
          typename Hash,
          typename Pred,
          typename KeyOfValue>
class HashTableConstIterator
    : public std::iterator<std::forward_iterator_tag, const T> {
 public:
  using _HashTable = RegionBasedHashTable<Key, T, Hash, Pred, KeyOfValue>;
  using _HashTableNode = HashTableNode<T>;
  using _Base = std::iterator<std::forward_iterator_tag, const T>;

  HashTableConstIterator();
  HashTableConstIterator(HashTableIterator<Key, T, Hash, Pred, KeyOfValue> it);
  HashTableConstIterator(const _HashTable* ht, const _HashTableNode* node);
  HashTableConstIterator& operator++();
  HashTableConstIterator operator++(int);
  typename _Base::reference operator*() const;
  typename _Base::pointer operator->() const;
  bool operator==(const HashTableConstIterator& rhs) const;
  bool operator!=(const HashTableConstIterator& rhs) const;

 private:
  const _HashTable* ht_;
  const _HashTableNode* node_;
};

template <typename Key,
          typename T,
          typename Hash,
          typename Pred,
          typename KeyOfValue>
class RegionBasedHashTable<
    Key,
    T,
    Hash,
    Pred,
    KeyOfValue,
    typename std::enable_if<std::is_pod<T>::value &&
                            !std::is_pointer<T>::value>::type> {
 public:
  using key_type = Key;
  using value_type = T;
  using hasher = Hash;
  using key_equal = Pred;
  using iterator = HashTableIterator<Key, T, Hash, Pred, KeyOfValue>;
  using const_iterator = HashTableConstIterator<Key, T, Hash, Pred, KeyOfValue>;
  using local_iterator = iterator;
  using const_local_iterator = const_iterator;
  using size_type = uint32_t;
  using _HashTableNode = HashTableNode<T>;
  using _HashTable = RegionBasedHashTable<Key, T, Hash, Pred, KeyOfValue, void>;

  static std::unique_ptr<_HashTable> Create(char* data, size_t size);
  static std::unique_ptr<_HashTable> Restore(char* data, size_t size);

  bool empty() const;
  size_type size() const;
  size_type max_size() const;
  size_type bucket_count() const;
  size_type bucket(const key_type& k) const;
  size_type bucket_size(size_type n) const;

  iterator begin();
  const_iterator begin() const;
  const_iterator cbegin() const;
  // local_iterator begin(size_type n);
  iterator end();
  const_iterator end() const;
  const_iterator cend() const;
  // local_iterator end(size_type n);

  std::pair<iterator, bool> insert(const value_type& obj);
  // 外层使用模板using, 没法写这个operator=
  // value_type& operator=(const key_type& key);
  iterator erase(const_iterator position);
  size_type erase(const key_type& k);
  void clear();

  iterator find(const key_type& k);
  const_iterator find(const key_type& k) const;

 private:
  friend class HashTableIterator<Key, T, Hash, Pred, KeyOfValue>;
  friend class HashTableConstIterator<Key, T, Hash, Pred, KeyOfValue>;
  using VectorType = RegionBasedVector<NodeID>;
  using AllocatorType = RegionBasedAllocator<_HashTableNode>;
  struct Header {
    uint64_t magic;
    uint32_t element_size;
    uint32_t bucket_num;
    typename VectorType::Header bucket_header;
    typename AllocatorType::Header allocator_header;
  };

  static const uint64_t kMagic = 0x6d17ffb6f6d53d7e;
  RegionBasedHashTable();
  Header* header_;
  std::unique_ptr<VectorType> buckets_;
  std::unique_ptr<AllocatorType> alloc_;
};
}

#include "RegionBasedHashTable-inl.h"

#endif /* ----- #ifndef __ALPHA_REGION_BASED_HASH_TABLE_H__  ----- */
