/*
 * ==============================================================================
 *
 *       Filename:  main.cc
 *        Created:  04/26/15 15:15:04
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#include <cassert>
#include <iostream>
#include <vector>
#include <algorithm>
#include "RegionBasedHashTable.h"
//#include "RegionBasedVector.h"
//
template<typename First, typename Second>
struct PODPair {
  using first_type = First;
  using second_type = Second;
  First first;
  Second second;
};

template<typename Pair>
struct Select1st : std::unary_function<Pair, typename Pair::first_type> {
  const typename Pair::first_type& operator()(const Pair& p) const {
    return p.first;
  }
};

struct CombatantLite {
  uint16_t pos;
  uint16_t level;
  uint64_t time;
};

int main() {
#if 1
  using key_type = int;
  using value_type = PODPair<key_type, uint32_t>;
  using MapType = RegionBasedHashTable<key_type,
                          value_type,
                          std::hash<key_type>,
                          std::equal_to<key_type>,
                          Select1st<value_type>>;
  std::vector<char> buf(1 << 9);
  auto ht = MapType::Create(buf.data(), buf.size());
  assert (ht);
  assert (ht->empty());
  assert (ht->size() == 0);
  assert (ht->begin() == ht->end());

  auto res = ht->insert(value_type{1, 2});
  assert (res.second);
  assert (ht->size() == 1);
  assert (ht->empty() == false);
  assert (res.first->first == 1);
  assert (res.first->second == 2);
  auto it = ht->find(1);
  assert (res.first == it);
  it = ht->find(0);
  assert (it == ht->end());

  key_type k = 1 + ht->bucket_count();
  res = ht->insert(value_type{k,
        std::numeric_limits<value_type::second_type>::min()});
  auto iter = res.first;
  assert (res.second);
  assert (ht->size() == 2);
  assert (ht->bucket_size(ht->bucket(1)) == 2);

  k = 2;
  res = ht->insert(value_type{k,
        std::numeric_limits<value_type::second_type>::max()});
  assert (res.second);
  assert (ht->size() == 3);
  assert (ht->bucket_size(ht->bucket(k)) == 1);
  assert (ht->bucket_size(ht->bucket(0)) == 0);
  auto n = ht->erase(1);
  assert (n == 1);
  assert (ht->size() == 2);
  assert (ht->bucket_size(ht->bucket(1)) == 1);
  it = ht->find(1);
  assert (it == ht->end());

  //test iterator invalidation
  assert (ht->find(1 + ht->bucket_count()) == iter);

  ht->clear();
  try {
    for (int i = 0;; ++i) {
      ht->insert(value_type{i, static_cast<uint32_t>(i)});
    }
  } catch(std::bad_alloc& e) {
  }
  assert (ht->size() == ht->max_size());
  std::for_each(ht->cbegin(), ht->cend(), [](const value_type& v) {
      std::cout << v.first << " -> " << v.second << '\n';
  });

  std::cout << std::string(80, '*') << '\n';

  std::vector<char> newBuf = buf;
  auto newHt = MapType::Restore(newBuf.data(), newBuf.size());
  assert (newHt);
  assert (ht->size() == newHt->size());
  assert (ht->max_size() == newHt->max_size());
  auto size = newHt->size();
  std::for_each(newHt->cbegin(), newHt->cend(), [size](const value_type& v) {
      static size_t num = 0;
      std::cout << v.first << " -> " << v.second << '\n';
      assert (v.first == static_cast<int>(v.second));
      assert (++num <= size);
  });
#endif

#if 0
  std::vector<char> buf(40 * (1 << 20));
  using VectorType = RegionBasedVector<int>;
  VectorType::Header header;
  auto data = Align(buf.data());
  auto sz = buf.size() - (data - buf.data());
  auto p = VectorType::Create(data + 1, sz - 1, &header);
  assert (p == nullptr); //unaligned memory
  p = VectorType::Create(data, sz, &header);
  auto& v = *p;
  assert (v.size() == 0);
  assert (v.empty());
  std::cout << v.max_size() << '\n';
  v.push_back(1024);
  assert (v.size() == 1);
  assert (!v.empty());
  assert (v[0] == 1024);
  assert (v.front() == 1024);
  assert (v.back() == 1024);
  assert (v.at(0) == 1024);
  v.push_back(500);
  assert (v.size() == 2);
  assert (v[1] == 500);
  assert (v.at(1) == 500);
  assert (v.front() == 1024);
  assert (v.back() == 500);
  v.push_back(200);
  v.on_destroy([](int& p) {
      std::cout << "Erasing " << p << '\n';
  });
  auto it = std::find(v.begin(), v.end(), 500);
  assert (it != v.end());
  auto next = v.erase(it);
  assert (*next == 200);
  it = std::find(v.begin(), v.end(), 0);
  assert (it == v.end());
  assert (v.size() == 2);
  assert (v[0] == 1024);
  assert (v[1] == 200);
  assert (v.front() == 1024);
  assert (v.back() == 200);
  v.erase(v.begin());
  assert (v.size() == 1);
  assert (v.front() == v.back());
  assert (v.front() == 200);
  VectorType::Header new_header = header;
  auto n = VectorType::Restore(data, sz, &new_header);
  assert (n);
  assert (*n == v);
  try {
    (*n).at(n->size()) = 5;
    assert (false);
  } catch (std::out_of_range& e) {
  }
  std::fill(n->begin(), n->end(), 0);
  assert (n->front() == 0);
#endif
}
