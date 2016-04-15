/*
 * =============================================================================
 *
 *       Filename:  main.cc
 *        Created:  03/17/16 17:08:46
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <iostream>
#include <vector>
#include <alpha/experimental/RegionBasedHashSet.h>

int main() {
  std::vector<char> buf(1 << 27);
  using MySet = alpha::RegionBasedHashSet<unsigned>;
  auto s = MySet::Create(buf.data(), buf.size());
  s->insert(1);
  std::cout << s->size() << '\t' << s->max_size() << '\n';
}
