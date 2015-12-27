/*
 * =============================================================================
 *
 *       Filename:  tcp_connection_buffer.cc
 *        Created:  04/05/15 12:28:24
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "tcp_connection_buffer.h"
#include <cassert>
#include <cstring>
#include "compiler.h"

namespace alpha {
const size_t TcpConnectionBuffer::kMaxBufferSize = 1 << 20;  // 1MiB
TcpConnectionBuffer::TcpConnectionBuffer()
    : internal_buffer_(kDefaultBufferSize) {}

TcpConnectionBuffer::~TcpConnectionBuffer() = default;

size_t TcpConnectionBuffer::GetContiguousSpace() const {
  CheckIndex();
  return internal_buffer_.size() - write_index_;
}

char* TcpConnectionBuffer::WriteBegin() {
  CheckIndex();
  return &internal_buffer_[write_index_];
}

void TcpConnectionBuffer::AddBytes(size_t len) {
  write_index_ += len;
  CheckIndex();
}

bool TcpConnectionBuffer::Append(const alpha::Slice& data) {
  return Append(data.data(), data.size());
}

bool TcpConnectionBuffer::Append(const void* data, size_t size) {
  if (unlikely(!EnsureSpace(size))) {
    return false;
  }
  assert(GetContiguousSpace() >= size);
  ::memcpy(WriteBegin(), data, size);
  AddBytes(size);
  return true;
}

size_t TcpConnectionBuffer::SpaceBeforeFull() const {
  return kMaxBufferSize - internal_buffer_.size() + GetContiguousSpace();
}

alpha::Slice TcpConnectionBuffer::Read() const {
  CheckIndex();
  return alpha::Slice(&internal_buffer_[read_index_],
                      write_index_ - read_index_);
}

size_t TcpConnectionBuffer::ReadAndClear(void* buf, size_t len) {
  alpha::Slice data = Read();
  size_t n = std::min(len, data.size());
  ::memcpy(buf, data.data(), n);
  ConsumeBytes(n);
  return n;
}

void TcpConnectionBuffer::ConsumeBytes(size_t n) {
  read_index_ += n;
  CheckIndex();
  if (read_index_ == write_index_) {
    read_index_ = write_index_ = 0;  //重置位置
  }
}

bool TcpConnectionBuffer::EnsureSpace(size_t n) {
  if (unlikely(n > kMaxBufferSize)) {
    return false;
  }

  size_t space_left = GetContiguousSpace();
  if (space_left < n) {
    size_t space_more = (n - space_left) * 2;
    auto new_size =
        std::min(kMaxBufferSize, internal_buffer_.size() + space_more);
    if (unlikely(new_size - internal_buffer_.size() + space_left < n)) {
      return false;
    }
    internal_buffer_.resize(new_size);
  }
  return true;
}

void TcpConnectionBuffer::CheckIndex() const {
  assert(read_index_ <= write_index_);
  assert(write_index_ <= internal_buffer_.size());
}
}
