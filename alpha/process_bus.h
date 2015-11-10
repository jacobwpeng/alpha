/*
 * ==============================================================================
 *
 *       Filename:  process_bus.h
 *        Created:  12/21/14 14:03:24
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#ifndef __PROCESS_BUS_H__
#define __PROCESS_BUS_H__

#include <memory>
#include "ring_buffer.h"
#include "mmap_file.h"

namespace alpha {
class ProcessBus {
 private:
  struct Header {
    const static int64_t kMagicNumber = 7316964413220353303;
    int64_t magic_number;
  };
  const static size_t kHeaderSize = sizeof(Header);

 private:
  ProcessBus();

 public:
  static std::unique_ptr<ProcessBus> RestoreOrCreate(
      const alpha::Slice& filepath, size_t size, bool force = false);
  static std::unique_ptr<ProcessBus> RestoreFrom(const alpha::Slice& filepath,
                                                 size_t size);
  static std::unique_ptr<ProcessBus> CreateFrom(const alpha::Slice& filepath,
                                                size_t size);
  static const size_t kMaxBufferBodyLength = RingBuffer::kMaxBufferBodyLength;

  bool Write(const char* buf, int len);
  char* Read(int* plen);

  bool empty() const;
  std::string filepath() const;

 private:
  std::string filepath_;
  Header* header_;
  std::unique_ptr<RingBuffer> buf_;
  std::unique_ptr<MMapFile> file_;
};
}

#endif /* ----- #ifndef __PROCESS_BUS_H__  ----- */
