/*
 * ==============================================================================
 *
 *       Filename:  ProcessBus.cc
 *        Created:  12/21/14 14:05:03
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#include <alpha/ProcessBus.h>

#include <cassert>
#include <alpha/logger.h>

namespace alpha {

ProcessBus::ProcessBus(ProcessBus&& other) { swap(other); }

ProcessBus& ProcessBus::operator=(ProcessBus&& other) {
  swap(other);
  return *this;
}

bool ProcessBus::CreateFrom(alpha::Slice filepath, int64_t size,
                            QueueOrder order) {
  MemoryMappedFile mapped_file;
  if (!mapped_file.Init(filepath, size,
                        MemoryMappedFlags::kCreateIfNotExists)) {
    return false;
  }

  int64_t read_queue_size = size / 2;
  void* read_queue_start = mapped_file.mapped_start();
  int64_t write_queue_size = size - read_queue_size;
  void* write_queue_start =
      reinterpret_cast<uint8_t*>(mapped_file.mapped_start()) + read_queue_size;

  if (order == QueueOrder::kWriteFirst) {
    std::swap(read_queue_size, write_queue_size);
    std::swap(read_queue_start, write_queue_start);
  }
  RingBuffer read_queue, write_queue;
  if (!read_queue.CreateFrom(read_queue_start, read_queue_size)) {
    return false;
  }
  if (!write_queue.CreateFrom(write_queue_start, write_queue_size)) {
    return false;
  }
  mmaped_file_ = std::move(mapped_file);
  read_queue_ = std::move(read_queue);
  write_queue_ = std::move(write_queue);
  return true;
}

bool ProcessBus::RestoreFrom(alpha::Slice filepath, QueueOrder order) {
  MemoryMappedFile mapped_file;
  if (!mapped_file.Init(filepath, 0, MemoryMappedFlags::kCreateIfNotExists)) {
    return false;
  }

  const int64_t size = mapped_file.size();
  int64_t read_queue_size = size / 2;
  void* read_queue_start = mapped_file.mapped_start();
  int64_t write_queue_size = size - read_queue_size;
  void* write_queue_start =
      reinterpret_cast<uint8_t*>(mapped_file.mapped_start()) + read_queue_size;

  if (order == QueueOrder::kWriteFirst) {
    std::swap(read_queue_size, write_queue_size);
    std::swap(read_queue_start, write_queue_start);
  }
  RingBuffer read_queue, write_queue;
  if (!read_queue.RestoreFrom(read_queue_start, read_queue_size)) {
    return false;
  }
  if (!write_queue.RestoreFrom(write_queue_start, write_queue_size)) {
    return false;
  }

  mmaped_file_ = std::move(mapped_file);
  read_queue_ = std::move(read_queue);
  write_queue_ = std::move(write_queue);
  return true;
}

bool ProcessBus::RestoreOrCreate(alpha::Slice filepath, int64_t size,
                                 QueueOrder order, bool force) {
  if (RestoreFrom(filepath, order)) {
    return true;
  } else if (force) {
    return CreateFrom(filepath, size, order);
  } else {
    return false;
  }
}

bool ProcessBus::Write(const void* buf, int len) {
  DCHECK(write_queue_);
  return write_queue_.Push(buf, len);
}

void* ProcessBus::Read(int* plen) {
  DCHECK(read_queue_);
  return read_queue_.Pop(plen);
}

void* ProcessBus::Peek(int* plen) {
  DCHECK(read_queue_);
  return read_queue_.Peek(plen);
}

void ProcessBus::swap(ProcessBus& other) {
  std::swap(mmaped_file_, other.mmaped_file_);
  std::swap(read_queue_, other.read_queue_);
  std::swap(write_queue_, other.write_queue_);
}

ProcessBus::operator bool() const { return mmaped_file_; }

// bool ProcessBus::empty() const {
//  DCHECK(ring_buffer_);
//  return ring_buffer_.empty();
//}

std::string ProcessBus::filepath() const { return mmaped_file_.filepath(); }
}
