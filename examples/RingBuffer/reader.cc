/*
 * =============================================================================
 *
 *       Filename:  reader.cc
 *        Created:  12/04/16 00:31:24
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <cstdio>
#include <alpha/logger.h>
#include <alpha/MemoryMappedFile.h>
#include <alpha/RingBuffer.h>

int main(int argc, char* argv[]) {
  if (argc != 3) {
    printf("Usage: %s FILE SIZE\n", argv[0]);
    return EXIT_FAILURE;
  }
  int64_t size = std::stoul(argv[2]);
  alpha::Logger::Init(argv[0]);
  alpha::Logger::set_logtostderr(true);
  alpha::MemoryMappedFile file;
  bool ok =
      file.Init(argv[1], size, alpha::MemoryMappedFlags::kCreateIfNotExists);
  CHECK(ok);

  alpha::RingBuffer buffer;
  ok = buffer.CreateFrom(file.mapped_start(), size);
  CHECK(ok);

  while (1) {
    int len;
    void* p = buffer.Pop(&len);
    if (p) {
      LOG_INFO << "sz: " << len;
    } else {
      ::usleep(1000 * 10);
    }
  }
}
