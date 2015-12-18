/*
 * =============================================================================
 *
 *       Filename:  RegionBasedHashTable-inl.h
 *        Created:  07/07/15 15:10:47
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __ALPHA_REGION_BASED_HASH_TABLE_H__
#error This file should only be included from RegionBasedHashTable.h
#endif

#define HashTableIteratorTypeDeclaration                            \
  template <typename Key, typename T, typename Hash, typename Pred, \
            typename KeyOfValue>
#define HashTableIteratorType HashTableIterator<Key, T, Hash, Pred, KeyOfValue>

namespace alpha {

HashTableIteratorTypeDeclaration HashTableIteratorType::HashTableIterator()
    : ht_(nullptr), node_(nullptr) {}
HashTableIteratorTypeDeclaration HashTableIteratorType::HashTableIterator(
    _HashTable* ht, _HashTableNode* node)
    : ht_(ht), node_(node) {}
HashTableIteratorTypeDeclaration HashTableIteratorType& HashTableIteratorType::
operator++() {
  auto bucket_index = ht_->bucket(KeyOfValue()(node_->val));
  auto next = node_->next;
  auto index = bucket_index + 1;
  if (next != _HashTable::AllocatorType::kInvalidNodeID) {
    node_ = ht_->alloc_->Get(next);
    return *this;
  }
  for (auto index = bucket_index + 1; index < ht_->bucket_count(); ++index) {
    next = (*ht_->buckets_)[index];
    if (next != _HashTable::AllocatorType::kInvalidNodeID) {
      node_ = ht_->alloc_->Get(next);
      return *this;
    }
  }
  node_ = nullptr;
  return *this;
}
HashTableIteratorTypeDeclaration HashTableIteratorType HashTableIteratorType::
operator++(int) {
  auto tmp = *this;
  ++*this;
  return tmp;
}
HashTableIteratorTypeDeclaration typename HashTableIteratorType::_Base::
    reference HashTableIteratorType::
    operator*() const {
  return node_->val;
}

HashTableIteratorTypeDeclaration typename HashTableIteratorType::_Base::pointer
    HashTableIteratorType::
    operator->() const {
  return &(operator*());
}

HashTableIteratorTypeDeclaration bool HashTableIteratorType::operator==(
    const HashTableIterator& rhs) const {
  return ht_ == rhs.ht_ && node_ == rhs.node_;
}

HashTableIteratorTypeDeclaration bool HashTableIteratorType::operator!=(
    const HashTableIterator& rhs) const {
  return !(*this == rhs);
}

#undef HashTableIteratorType
#undef HashTableIteratorTypeDeclaration

#define HashTableIteratorTypeDeclaration                            \
  template <typename Key, typename T, typename Hash, typename Pred, \
            typename KeyOfValue>
#define HashTableIteratorType \
  HashTableConstIterator<Key, T, Hash, Pred, KeyOfValue>

HashTableIteratorTypeDeclaration HashTableIteratorType::HashTableConstIterator()
    : ht_(nullptr), node_(nullptr) {}

HashTableIteratorTypeDeclaration HashTableIteratorType::HashTableConstIterator(
    HashTableIterator<Key, T, Hash, Pred, KeyOfValue> it)
    : ht_(it.ht_), node_(it.node_) {}

HashTableIteratorTypeDeclaration HashTableIteratorType::HashTableConstIterator(
    const _HashTable* ht, const _HashTableNode* node)
    : ht_(ht), node_(node) {}
HashTableIteratorTypeDeclaration HashTableIteratorType& HashTableIteratorType::
operator++() {
  auto bucket_index = ht_->bucket(KeyOfValue()(node_->val));
  auto next = node_->next;
  auto index = bucket_index + 1;
  if (next != _HashTable::AllocatorType::kInvalidNodeID) {
    node_ = ht_->alloc_->Get(next);
    return *this;
  }
  for (auto index = bucket_index + 1; index < ht_->bucket_count(); ++index) {
    next = (*ht_->buckets_)[index];
    if (next != _HashTable::AllocatorType::kInvalidNodeID) {
      node_ = ht_->alloc_->Get(next);
      return *this;
    }
  }
  node_ = nullptr;
  return *this;
}
HashTableIteratorTypeDeclaration HashTableIteratorType HashTableIteratorType::
operator++(int) {
  auto tmp = *this;
  ++*this;
  return tmp;
}
HashTableIteratorTypeDeclaration typename HashTableIteratorType::_Base::
    reference HashTableIteratorType::
    operator*() const {
  return node_->val;
}

HashTableIteratorTypeDeclaration typename HashTableIteratorType::_Base::pointer
    HashTableIteratorType::
    operator->() const {
  return &(operator*());
}

HashTableIteratorTypeDeclaration bool HashTableIteratorType::operator==(
    const HashTableConstIterator& rhs) const {
  return ht_ == rhs.ht_ && node_ == rhs.node_;
}

HashTableIteratorTypeDeclaration bool HashTableIteratorType::operator!=(
    const HashTableConstIterator& rhs) const {
  return !(*this == rhs);
}

#undef HashTableIteratorType
#undef HashTableIteratorTypeDeclaration

#define HashTableType                                                     \
  RegionBasedHashTable<Key, T, Hash, Pred, KeyOfValue,                    \
                       typename std::enable_if < std::is_pod<T>::value && \
                           !std::is_pointer<T>::value>::type >
#define HashTableTypeDeclaration                                    \
  template <typename Key, typename T, typename Hash, typename Pred, \
            typename KeyOfValue>

static const uint32_t kNumPrimes = 28;
static const unsigned long kPrimeList[kNumPrimes] = {
    53ul,         97ul,         193ul,       389ul,       769ul,
    1543ul,       3079ul,       6151ul,      12289ul,     24593ul,
    49157ul,      98317ul,      196613ul,    393241ul,    786433ul,
    1572869ul,    3145739ul,    6291469ul,   12582917ul,  25165843ul,
    50331653ul,   100663319ul,  201326611ul, 402653189ul, 805306457ul,
    1610612741ul, 3221225473ul, 4294967291ul};

HashTableTypeDeclaration std::unique_ptr<typename HashTableType::_HashTable>
HashTableType::Create(char* data, size_t size) {
  static_assert(alignof(Header) <= kAlignmentRequired,
                "kAlignmentRequired is not enough!");
  if (CheckAligned(data) == false) {
    return nullptr;
  }
  auto p = data;
  auto aligned_header = p;
  p += sizeof(Header);
  // estimate max_size
  // max load factor is 2
  // (max_size / 2) * sizeof(NodeID) + (max_size) * sizeof(_HashTableNode) =
  // size;
  auto bucket_num =
      std::max(kPrimeList[0],
               (2 * size) / (sizeof(NodeID) + 2 * sizeof(_HashTableNode)));
  for (auto i = 0u; i < kNumPrimes; ++i) {
    if (bucket_num <= kPrimeList[i]) {
      bucket_num = kPrimeList[i];
      break;
    }
  }
  auto aligned_bucket_start = Align(p);
  auto bucket_size = bucket_num * sizeof(NodeID);
  p = aligned_bucket_start + bucket_size;
  size_t min_size = (p - data);
  if (size < min_size) {
    return nullptr;
  }
  std::unique_ptr<typename HashTableType::_HashTable> ht(
      new typename HashTableType::_HashTable);
  ht->header_ = reinterpret_cast<Header*>(aligned_header);
  ht->header_->magic = kMagic;
  ht->header_->element_size = sizeof(value_type);
  ht->header_->bucket_num = bucket_num;
  ht->buckets_ = VectorType::Create(aligned_bucket_start, bucket_size,
                                    &ht->header_->bucket_header);
  assert(ht->buckets_);
  assert(ht->buckets_->max_size() == bucket_num);
  for (size_t i = 0; i < bucket_num; ++i) {
    ht->buckets_->push_back(AllocatorType::kInvalidNodeID);
  }
  p = Align(p);
  if (data + size < p) {
    return nullptr;
  }
  size_t sz = data + size - p;
  ht->alloc_ = AllocatorType::Create(p, sz, &ht->header_->allocator_header);
  assert(ht->alloc_);
  return ht;
}

HashTableTypeDeclaration std::unique_ptr<typename HashTableType::_HashTable>
HashTableType::Restore(char* data, size_t size) {
  if (CheckAligned(data) == false) {
    return nullptr;
  }
  auto p = data;
  auto header = reinterpret_cast<Header*>(p);
  p += sizeof(Header);
  auto it = std::find(std::begin(kPrimeList), std::end(kPrimeList),
                      header->bucket_num);
  if (header->magic != kMagic || header->element_size != sizeof(value_type) ||
      it == std::end(kPrimeList)) {
    return nullptr;
  }
  auto aligned_bucket_start = Align(p);
  auto bucket_size = header->bucket_num * sizeof(NodeID);
  p = aligned_bucket_start + bucket_size;
  p = Align(p);
  size_t min_size = (p - data);
  if (size < min_size) {
    return nullptr;
  }
  auto buckets = VectorType::Restore(aligned_bucket_start, bucket_size,
                                     &header->bucket_header);
  if (buckets == nullptr) {
    return nullptr;
  }
  auto sz = (data + size - p);
  auto alloc = AllocatorType::Restore(p, sz, &header->allocator_header);
  if (alloc == nullptr) {
    return nullptr;
  }
  std::unique_ptr<typename HashTableType::_HashTable> ht(
      new typename HashTableType::_HashTable);
  ht->header_ = header;
  ht->buckets_ = std::move(buckets);
  ht->alloc_ = std::move(alloc);
  return ht;
}

HashTableTypeDeclaration bool HashTableType::empty() const {
  return size() == 0;
}

HashTableTypeDeclaration typename HashTableType::size_type HashTableType::size()
    const {
  return alloc_->size();
}

HashTableTypeDeclaration typename HashTableType::size_type
HashTableType::max_size() const {
  return alloc_->max_size();
}

HashTableTypeDeclaration typename HashTableType::size_type
HashTableType::bucket_count() const {
  return buckets_->size();
}

HashTableTypeDeclaration typename HashTableType::size_type
HashTableType::bucket_size(size_type n) const {
  assert(n < bucket_count());
  auto num = 0;
  auto id = (*buckets_)[n];
  while (id != AllocatorType::kInvalidNodeID) {
    ++num;
    id = alloc_->Get(id)->next;
  }
  return num;
}

HashTableTypeDeclaration typename HashTableType::size_type
HashTableType::bucket(const key_type& k) const {
  auto hash = hasher()(k);
  return hash % buckets_->size();
}

HashTableTypeDeclaration typename HashTableType::iterator
HashTableType::begin() {
  for (size_t i = 0; i < buckets_->size(); ++i) {
    if ((*buckets_)[i] != AllocatorType::kInvalidNodeID) {
      return iterator(this, alloc_->Get((*buckets_)[i]));
    }
  }
  return end();
}

HashTableTypeDeclaration typename HashTableType::const_iterator
HashTableType::begin() const {
  for (size_t i = 0; i < buckets_->size(); ++i) {
    if ((*buckets_)[i] != AllocatorType::kInvalidNodeID) {
      return const_iterator(this, alloc_->Get((*buckets_)[i]));
    }
  }
  return end();
}

HashTableTypeDeclaration typename HashTableType::const_iterator
HashTableType::cbegin() const {
  return begin();
}

// HashTableTypeDeclaration
// typename HashTableType::local_iterator HashTableType::begin(size_type n) {
//  assert (n < bucket_count());
//  auto id = (*buckets_)[n];
//  if (id == AllocatorType::kInvalidNodeID) {
//    return local_iterator(this, nullptr);
//  } else {
//    return local_iterator(this, alloc_->Get(id));
//  }
//}

HashTableTypeDeclaration typename HashTableType::iterator HashTableType::end() {
  return iterator(this, nullptr);
}

HashTableTypeDeclaration typename HashTableType::const_iterator
HashTableType::end() const {
  return const_iterator(this, nullptr);
}

HashTableTypeDeclaration typename HashTableType::const_iterator
HashTableType::cend() const {
  return end();
}

// HashTableTypeDeclaration
// typename HashTableType::local_iterator HashTableType::end(size_type n) {
//  assert (n < bucket_count());
//  return end();
//}

HashTableTypeDeclaration std::pair<typename HashTableType::iterator, bool>
HashTableType::insert(const value_type& obj) {
  auto key = KeyOfValue()(obj);
  auto bucket_index = bucket(key);
  auto id = (*buckets_)[bucket_index];
  while (id != AllocatorType::kInvalidNodeID) {
    auto node = alloc_->Get(id);
    if (key_equal()(KeyOfValue()(node->val), key)) {
      return std::make_pair(iterator(this, node), false);
    } else {
      id = node->next;
    }
  }
  id = alloc_->Allocate();
  auto node = alloc_->Get(id);
  node->val = obj;
  node->next = (*buckets_)[bucket_index];
  (*buckets_)[bucket_index] = id;
  return std::make_pair(iterator(this, node), true);
}

HashTableTypeDeclaration typename HashTableType::size_type HashTableType::erase(
    const key_type& k) {
  auto bucket_index = bucket(k);
  auto id = (*buckets_)[bucket_index];
  auto prev = &(*buckets_)[bucket_index];
  while (id != AllocatorType::kInvalidNodeID) {
    auto node = alloc_->Get(id);
    if (key_equal()(KeyOfValue()(node->val), k)) {
      *prev = node->next;
      alloc_->Deallocate(id);
      return 1;
    } else {
      prev = &node->next;
      id = node->next;
    }
  }
  return 0;
}

HashTableTypeDeclaration void HashTableType::clear() {
  std::fill(buckets_->begin(), buckets_->end(), AllocatorType::kInvalidNodeID);
  alloc_->clear();
}

HashTableTypeDeclaration typename HashTableType::iterator HashTableType::find(
    const key_type& k) {
  auto bucket_index = bucket(k);
  auto id = (*buckets_)[bucket_index];
  while (id != AllocatorType::kInvalidNodeID) {
    auto node = alloc_->Get(id);
    if (key_equal()(KeyOfValue()(node->val), k)) {
      return iterator(this, node);
    } else {
      id = node->next;
    }
  }
  return end();
}

HashTableTypeDeclaration HashTableType::RegionBasedHashTable()
    : header_(nullptr) {}
}

#undef HashTableTypeDeclaration
#undef HashTableType
