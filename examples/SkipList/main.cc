/*
 * ==============================================================================
 *
 *       Filename:  main.cc
 *        Created:  03/29/15 13:45:11
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#include <map>
#include <vector>
#include <alpha/SkipList.h>
#include <alpha/Logger.h>
#include <alpha/Random.h>

int main(int argc, char* argv[]) {
  alpha::Logger::Init(argv[0]);
  alpha::Logger::set_logtostderr(true);
  using MapType = alpha::SkipList<uint32_t, uint32_t>;
  std::vector<char> buf(1 << 20);
  auto m = MapType::Create(buf.data(), buf.size());
  CHECK(m);

  auto p = m->insert(std::make_pair(1024, 1));
  CHECK(p.second);
  CHECK(p.first->second == 1);

  auto it = m->find(1024);
  CHECK(it != m->end());
  CHECK(it->first == 1024);
  CHECK(it->second == 1);

  m->erase(it);

  LOG_INFO << "After erase, size: " << m->size();
}
