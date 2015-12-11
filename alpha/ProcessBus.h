/*
 * ==============================================================================
 *
 *       Filename:  ProcessBus.h
 *        Created:  12/21/14 14:03:24
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#ifndef __ALPHA_PROCESS_BUS_H__
#define __ALPHA_PROCESS_BUS_H__

#include <memory>
#include "RingBuffer.h"
#include "MMapFile.h"

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
  static std::unique_ptr<ProcessBus> RestoreOrCreate(alpha::Slice filepath,
                                                     size_t size,
                                                     bool force = false);
  static std::unique_ptr<ProcessBus> RestoreFrom(alpha::Slice filepath);
  static std::unique_ptr<ProcessBus> CreateFrom(alpha::Slice filepath,
                                                size_t size);
  static const size_t kMaxBufferBodyLength = RingBuffer::kMaxBufferBodyLength;

  bool Write(const char* buf, int len);
  char* Read(int* plen);

  bool empty() const;
  std::string filepath() const;

 private:
  std::string filepath_;
  Header* header_;
  std::unique_ptr<MMapFile> file_;
  std::unique_ptr<RingBuffer> buf_;
};
}

#endif /* ----- #ifndef __ALPHA_PROCESS_BUS_H__  ----- */
