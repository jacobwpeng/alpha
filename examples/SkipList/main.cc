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
#include <alpha/skip_list.h>
#include <alpha/logger.h>
#include <alpha/random.h>

int main(int argc, char* argv[]) {
  std::vector<char> buf(1 << 27);
  alpha::Logger::Init(argv[0]);
  using MapType = alpha::SkipList<uint32_t, uint32_t>;
  auto m = MapType::Create(buf.data(), buf.size());
  std::map<uint32_t, uint32_t> benchmark;

  uint64_t loops = 0;
  while (1) {
    auto op = alpha::Random::Rand32(3);
    auto key = alpha::Random::Rand32(m->max_size());
    if (op == 0 && m->size() != m->max_size()) {
      auto val = alpha::Random::Rand32();
      auto p = std::make_pair(key, val);
      auto res = m->insert(p);
      auto benchmark_res = benchmark.insert(p);
      if (unlikely(res.second != benchmark_res.second)) {
        LOG_ERROR << "Insert failed";
        return EXIT_FAILURE;
      }
    } else if (op == 1) {
      if (unlikely((m->find(key) == m->end()) !=
                   (benchmark.find(key) == benchmark.end()))) {
        LOG_ERROR << "Find failed";
        return EXIT_FAILURE;
      }
    } else {
      if (unlikely(m->erase(key) != benchmark.erase(key))) {
        LOG_ERROR << "Erase failed";
        return EXIT_FAILURE;
      }
    }
    LOG_INFO_IF(++loops % 1000000 == 0) << loops;
  }
}
