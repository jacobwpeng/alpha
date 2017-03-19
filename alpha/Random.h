/*
 * =============================================================================
 *
 *       Filename:  Random.h
 *        Created:  05/07/15 19:32:56
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __RANDOM_H__
#define __RANDOM_H__

#include <random>
#include <algorithm>
#include <functional>
#include <type_traits>
#include <alpha/Compiler.h>
#include <alpha/ScopedGeneric.h>

namespace alpha {

class BufferedRandomDevice {
 public:
  explicit BufferedRandomDevice();
  void Get(void* data, size_t size);

 private:
  alpha::ScopedFILE file_;
};

class ThreadLocalPRNG {
 public:
  ThreadLocalPRNG();
  using result_type = uint32_t;
  result_type operator()() { return impl(local_); }

  static constexpr result_type min() {
    return std::numeric_limits<result_type>::min();
  }

  static constexpr result_type max() {
    return std::numeric_limits<result_type>::max();
  }

 private:
  class LocalInstancePRNG;
  LocalInstancePRNG* local_;
  static result_type impl(LocalInstancePRNG* rng);
};

class Random {
 public:
  using DefaultRNG = std::mt19937;
  template <typename RNG>
  using ValidRNG = typename std::enable_if<
      std::is_unsigned<typename std::result_of<RNG&()>::type>::value,
      RNG>::type;

  template <typename RNG = DefaultRNG>
  static ValidRNG<RNG> Create();

  template <typename RNG = DefaultRNG>
  static void Seed(ValidRNG<RNG>& rng);

  static void SecureRandom(void* data, size_t size);

  template <typename RNG = ThreadLocalPRNG>
  static uint32_t Rand32(uint32_t min,
                         uint32_t max,
                         ValidRNG<RNG> rng = RNG()) {
    if (min == max) {
      return 0;
    }
    return std::uniform_int_distribution<uint32_t>(min, max - 1)(rng);
  }

  template <typename RNG = ThreadLocalPRNG>
  static uint32_t Rand32(uint32_t max, ValidRNG<RNG> rng = RNG()) {
    return Rand32(0, max, rng);
  }

  template <typename RNG = ThreadLocalPRNG>
  static uint32_t Rand32(ValidRNG<RNG> rng = RNG()) {
    return Rand32(0, std::numeric_limits<uint32_t>::max(), rng);
  }

  template <typename RNG = ThreadLocalPRNG>
  static uint64_t Rand64(uint64_t min,
                         uint64_t max,
                         ValidRNG<RNG> rng = RNG()) {
    if (min == max) {
      return 0;
    }
    return std::uniform_int_distribution<uint64_t>(min, max - 1)(rng);
  }

  template <typename RNG = ThreadLocalPRNG>
  static uint64_t Rand64(uint64_t max, ValidRNG<RNG> rng = RNG()) {
    return Rand64(0, max, rng);
  }

  template <typename RNG = ThreadLocalPRNG>
  static uint64_t Rand64(ValidRNG<RNG> rng = RNG()) {
    return Rand64(0, std::numeric_limits<uint64_t>::max(), rng);
  }

  template <typename RandomAccessIterator, typename RNG = ThreadLocalPRNG>
  static void Shuffle(RandomAccessIterator first,
                      RandomAccessIterator last,
                      ValidRNG<RNG> rng = RNG()) {
    using namespace std::placeholders;
    auto gen = [&rng](uint32_t max) { return Random::Rand32(max, rng); };
    std::random_shuffle(first, last, gen);
  }

  template <typename InputIterator, typename RNG = ThreadLocalPRNG>
  static std::vector<InputIterator> Sample(InputIterator first,
                                           InputIterator last,
                                           size_t k,
                                           ValidRNG<RNG> rng = RNG()) {
    return SampleImpl(
        first,
        last,
        k,
        typename std::is_same<
            typename std::iterator_traits<InputIterator>::iterator_category,
            std::random_access_iterator_tag>::type(),
        rng);
  }

 private:
  template <typename InputIterator, typename RNG = ThreadLocalPRNG>
  static std::vector<InputIterator> SampleImpl(InputIterator first,
                                               InputIterator last,
                                               size_t k,
                                               std::true_type,
                                               ValidRNG<RNG> rng);

  template <typename InputIterator, typename RNG = ThreadLocalPRNG>
  static std::vector<InputIterator> SampleImpl(InputIterator first,
                                               InputIterator last,
                                               size_t k,
                                               std::false_type,
                                               ValidRNG<RNG> rng);
};
}

#include <alpha/Random-inl.h>

#endif /* ----- #ifndef __RANDOM_H__  ----- */
