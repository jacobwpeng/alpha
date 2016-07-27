/*
 * ==============================================================================
 *
 *       Filename:  RingBuffer.h
 *        Created:  12/21/14 13:53:38
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#pragma once

#include <memory>
#include <cstddef>
#include <alpha/compiler.h>

namespace alpha {
class RingBuffer {
 private:
  struct OffsetData {
    ptrdiff_t front_offset;
    ptrdiff_t back_offset;
  };

 public:
  static const int kMinByteSize;
  static const int kMaxBufferBodyLength = 1 << 16;  // 64KB

 public:
  RingBuffer();
  DISABLE_COPY_ASSIGNMENT(RingBuffer);

  RingBuffer(RingBuffer&& other);
  RingBuffer& operator=(RingBuffer&& other);

  bool CreateFrom(void* start, int64_t len);
  bool RestoreFrom(void* start, int64_t len);
  bool Push(const void* buf, int len);
  void* Pop(int* len);
  void* Peek(int* len);
  void swap(RingBuffer& other);

  int SpaceLeft() const;
  bool empty() const;
  operator bool() const;

 private:
  uint8_t* get_front() const;
  uint8_t* get_back() const;

  void set_front(uint8_t* front);
  void set_back(uint8_t* back);

  int NextBufferLength() const;
  void Write(const uint8_t* buf, int len);
  uint8_t* Read(int* plen, uint8_t** new_front);

 private:
  static const int64_t kBufferHeaderLength = sizeof(int);
  static const int64_t kMaxBufferLength =
      kBufferHeaderLength + kMaxBufferBodyLength;
  static const int64_t kExtraSpace = 1;
  uint8_t* data_start_;
  uint8_t* end_;
  volatile OffsetData* offset_;
};
}
