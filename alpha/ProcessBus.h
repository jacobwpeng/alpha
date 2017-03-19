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

#pragma once

#include <memory>
#include <alpha/RingBuffer.h>
#include <alpha/MemoryMappedFile.h>

namespace alpha {
class ProcessBus {
 public:
  static const size_t kMaxBufferBodyLength = RingBuffer::kMaxBufferBodyLength;
  enum class QueueOrder { kReadFirst = 0, kWriteFirst = 1 };

  ProcessBus() = default;

  ProcessBus(ProcessBus&& other);

  ProcessBus& operator=(ProcessBus&& other);

  bool CreateFrom(alpha::Slice filepath, int64_t size, QueueOrder order);

  bool RestoreFrom(alpha::Slice filepath, QueueOrder order);

  bool RestoreOrCreate(alpha::Slice filepath,
                       int64_t size,
                       QueueOrder order,
                       bool force = false);

  bool Write(const void* buf, int len);

  void* Read(int* plen);

  void* Peek(int* plen);

  void swap(ProcessBus& other);

  operator bool() const;

  // bool empty() const;

  std::string filepath() const;

 private:
  alpha::MemoryMappedFile mmaped_file_;
  alpha::RingBuffer read_queue_;
  alpha::RingBuffer write_queue_;
};
}
