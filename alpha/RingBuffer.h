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

#ifndef __ALPHA_RING_BUFFER_H__
#define __ALPHA_RING_BUFFER_H__

#include <memory>
#include <cstddef>
#include "compiler.h"

namespace alpha {
class RingBuffer {
 private:
  struct OffsetData {
    ptrdiff_t front_offset;
    ptrdiff_t back_offset;
  };

  RingBuffer();
  DISABLE_COPY_ASSIGNMENT(RingBuffer);

 public:
  static const int kMinByteSize;
  static const int kMaxBufferBodyLength = 1 << 16;  // 64KB

 public:
  static std::unique_ptr<RingBuffer> RestoreFrom(void* start, size_t len);
  static std::unique_ptr<RingBuffer> CreateFrom(void* start, size_t len);

  bool Push(const char* buf, int len);
  char* Pop(int* len);

  int SpaceLeft() const;
  bool empty() const;

 private:
  char* get_front() const;
  char* get_back() const;

  void set_front(char* front);
  void set_back(char* back);

  int NextBufferLength() const;
  void Write(const char* buf, int len);
  char* Read(int* plen);

 private:
  static const size_t kBufferHeaderLength = sizeof(int);
  static const size_t kMaxBufferLength =
      kBufferHeaderLength + kMaxBufferBodyLength;
  static const size_t kExtraSpace = 1;
  char* start_;
  char* end_;
  volatile OffsetData* offset_;
};
}

#endif /* ----- #ifndef __ALPHA_RING_BUFFER_H__  ----- */
