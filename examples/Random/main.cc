/*
 * =============================================================================
 *
 *       Filename:  main.cc
 *        Created:  05/08/15 13:10:22
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <iostream>
#include <alpha/random.h>
#include <alpha/logger.h>

int main(int argc, char* argv[]) {
  alpha::Logger::Init(argv[0]);
  std::vector<int> v;
  for (int i = 0; i < 100; ++i) {
    v.emplace_back(i);
  }
  alpha::Random::Shuffle(v.begin(), v.end());
  std::for_each(v.begin(), v.end(), [](int v) { LOG_INFO << v; });
}
