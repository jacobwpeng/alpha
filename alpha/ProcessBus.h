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
// 使用基于文件的mmap的共享内存来进行跨进程通信封装
// 内部使用单生产者，单消费者无锁的RingBuffer接口收发消息
// ProcessBus将整个文件映射的内存区域均分为两个部分
// 每个部分对应一个RingBuffer，A进程读消息，B进程写消息
class ProcessBus {
 public:
  static const size_t kMaxBufferBodyLength = RingBuffer::kMaxBufferBodyLength;
  enum class QueueOrder { kReadFirst = 0, kWriteFirst = 1 };

  ProcessBus() = default;

  ProcessBus(ProcessBus&& other);

  ProcessBus& operator=(ProcessBus&& other);

  bool CreateFrom(alpha::Slice filepath, int64_t size, QueueOrder order);

  bool RestoreFrom(alpha::Slice filepath, QueueOrder order);

  // 如果force为true, 则在restore失败的时候会自动truncate+清零对应文件
  bool RestoreOrCreate(alpha::Slice filepath,
                       int64_t size,
                       QueueOrder order,
                       bool force = true);

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
