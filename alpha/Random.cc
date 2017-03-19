/*
 * =============================================================================
 *
 *       Filename:  Random.cc
 *        Created:  05/08/15 13:03:35
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <alpha/Random.h>

namespace alpha {
BufferedRandomDevice::BufferedRandomDevice()
    : file_(::fopen("/dev/urandom", "rb")) {
  CHECK(file_.is_valid());
}

void BufferedRandomDevice::Get(void* data, size_t size) {
  auto nread = ::fread(data, 1, size, file_.get());
  CHECK(nread == size);
}

class ThreadLocalPRNG::LocalInstancePRNG {
 public:
  LocalInstancePRNG() : rng(Random::Create()) {}

  Random::DefaultRNG rng;
};

ThreadLocalPRNG::result_type ThreadLocalPRNG::impl(
    ThreadLocalPRNG::LocalInstancePRNG* local) {
  return local->rng();
}

ThreadLocalPRNG::ThreadLocalPRNG() {
  thread_local static LocalInstancePRNG local;
  local_ = &local;
}

void Random::SecureRandom(void* data, size_t size) {
  static thread_local BufferedRandomDevice d;
  d.Get(data, size);
}
}
