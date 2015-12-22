/*
 * =============================================================================
 *
 *       Filename:  RegionBasedHashMap.h
 *        Created:  12/16/15 17:15:49
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include "RegionBasedHashTable.h"

namespace alpha {
template <typename Pair>
struct Select1st : std::unary_function<Pair, typename Pair::first_type> {
  const typename Pair::first_type& operator()(const Pair& p) const {
    return p.first;
  }
};

template <typename First, typename Second>
struct PODPair {
  using first_type = First;
  using second_type = Second;
  First first;
  Second second;
};

template <typename F, typename S>
std::ostream& operator<<(std::ostream& os, const PODPair<F, S>& p) {
  os << "(" << p.first << ", " << p.second << ")";
  return os;
}

template <typename First, typename Second>
PODPair<First, Second> make_pod_pair(const First& first, const Second& second) {
  return {.first = first, .second = second};
}

template <class Key, class T, class Hash = std::hash<Key>,
          class Pred = std::equal_to<Key>>
using RegionBasedHashMap = RegionBasedHashTable<
    Key, PODPair<Key, T>, Hash, Pred, Select1st<PODPair<Key, T>>>;
}
