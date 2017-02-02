/*
 * ==============================================================================
 *
 *       Filename:  RingBuffer.cc
 *        Created:  12/21/14 13:56:42
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#include "RingBuffer.h"
#include <cassert>
#include <cstring>

namespace alpha {

namespace detail {
static __thread uint8_t local_buf[RingBuffer::kMaxBufferBodyLength];
}

const int RingBuffer::kMinByteSize =
    sizeof(RingBuffer::OffsetData) + RingBuffer::kExtraSpace;

RingBuffer::RingBuffer()
    : data_start_(nullptr), end_(nullptr), offset_(nullptr) {}

RingBuffer::RingBuffer(RingBuffer &&other) { swap(other); }

RingBuffer &RingBuffer::operator=(RingBuffer &&other) {
  swap(other);
  return *this;
}

bool RingBuffer::CreateFrom(void *start, int64_t len) {
  if (start == nullptr || len < kMinByteSize) return false;
  offset_ = reinterpret_cast<OffsetData *>(start);
  data_start_ = reinterpret_cast<uint8_t *>(start) + sizeof(OffsetData);
  end_ = reinterpret_cast<uint8_t *>(start) + len;
  set_front(data_start_);
  set_back(data_start_);
  return true;
}

bool RingBuffer::RestoreFrom(void *start, int64_t len) {
  if (start == nullptr || len < kMinByteSize) return false;
  offset_ = reinterpret_cast<OffsetData *>(start);
  data_start_ = reinterpret_cast<uint8_t *>(start) + sizeof(OffsetData);
  end_ = reinterpret_cast<uint8_t *>(start) + len;
  return true;
}

bool RingBuffer::Push(const void *buf, int len) {
  if (buf == nullptr || len == 0) return false;

  if (len > kMaxBufferBodyLength) return false;
  if (len > SpaceLeft()) return false;

  this->Write(reinterpret_cast<const uint8_t *>(buf), len);
  return true;
}

void *RingBuffer::Pop(int *plen) {
  assert(plen);
  if (empty()) {
    *plen = 0;
    return nullptr;
  } else {
    uint8_t *new_front;
    uint8_t *buf = Read(plen, &new_front);
    set_front(new_front);
    return buf;
  }
}

void *RingBuffer::Peek(int *plen) {
  assert(plen);
  if (empty()) {
    *plen = 0;
    return nullptr;
  } else {
    uint8_t *new_front;
    uint8_t *buf = Read(plen, &new_front);
    (void)new_front;
    return buf;
  }
}

void RingBuffer::swap(RingBuffer &other) {
  std::swap(data_start_, other.data_start_);
  std::swap(end_, other.end_);
  std::swap(offset_, other.offset_);
}

int RingBuffer::SpaceLeft() const {
  uint8_t *front = get_front();
  uint8_t *back = get_back();
  int result;
  if (back >= front) {
    result = end_ - back + front - data_start_ - kExtraSpace - sizeof(int32_t);
  } else {
    result = front - back - kExtraSpace - sizeof(int32_t);
  }
  return result < 0 ? 0 : result;
}

bool RingBuffer::empty() const { return get_front() == get_back(); }

RingBuffer::operator bool() const { return data_start_ != nullptr; }

uint8_t *RingBuffer::get_front() const {
  return offset_->front_offset + data_start_;
}

uint8_t *RingBuffer::get_back() const {
  return offset_->back_offset + data_start_;
}

void RingBuffer::set_front(uint8_t *front) {
  assert(front >= data_start_);
  offset_->front_offset = front - data_start_;
}

void RingBuffer::set_back(uint8_t *back) {
  assert(back >= data_start_);
  offset_->back_offset = back - data_start_;
}

void RingBuffer::Write(const uint8_t *buf, int len) {
  uint8_t *back = get_back();
  assert(end_ >= back);
  if (back + len + sizeof(len) <= end_) {
    // enough space
    memcpy(back, &len, sizeof(len));
    back += sizeof(len);
    memcpy(back, buf, len);
    back += len;
  } else {
    if (back + sizeof(len) <= end_) {
      // enough space for buffer length
      memcpy(back, &len, sizeof(len));
      back += sizeof(len);
      int first = end_ - back;
      memcpy(back, buf, first);
      memcpy(data_start_, buf + first, len - first);
      back = data_start_ + len - first;
    } else {
      uint8_t *len_addr = reinterpret_cast<uint8_t *>(&len);
      int first = end_ - back;
      memcpy(back, &len, first);
      memcpy(data_start_, len_addr + first, sizeof(len) - first);
      back = data_start_ + sizeof(len) - first;

      memcpy(back, buf, len);
      back += len;
    }
  }
  set_back(back);
}

uint8_t *RingBuffer::Read(int *plen, uint8_t **new_front) {
  assert(plen);
  auto *front = get_front();

  if (empty()) {
    *plen = 0;
    return nullptr;
  }

  const int buffer_len = NextBufferLength();
  assert(buffer_len > 0);
  assert(buffer_len <= RingBuffer::kMaxBufferBodyLength);
  *plen = buffer_len;
  auto *buf = detail::local_buf;
  uint8_t *content = front + RingBuffer::kBufferHeaderLength;

  if (content > end_) {
    ptrdiff_t offset = content - end_;
    memcpy(buf, data_start_ + offset, buffer_len);
    front = data_start_ + offset + buffer_len;
  } else if (content + buffer_len > end_) {
    ptrdiff_t tail_length = end_ - content;
    memcpy(buf, content, tail_length);
    memcpy(buf + tail_length, data_start_, buffer_len - tail_length);
    front = data_start_ + buffer_len - tail_length;
  } else {
    memcpy(buf, content, buffer_len);
    front = content + buffer_len;
  }
  *new_front = front;
  return reinterpret_cast<uint8_t *>(buf);
}

int RingBuffer::NextBufferLength() const {
  auto *front = get_front();

  assert(not empty());
  int len = 0;
  if (front + RingBuffer::kBufferHeaderLength <= end_) {
    len = *(reinterpret_cast<int *>(front));
  } else {
    ptrdiff_t offset = end_ - front;
    auto *len_addr = reinterpret_cast<uint8_t *>(&len);
    memcpy(len_addr, front, offset);
    memcpy(len_addr + offset, data_start_,
           RingBuffer::kBufferHeaderLength - offset);
  }
  return len;
}
}
