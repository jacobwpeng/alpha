/*
 * =============================================================================
 *
 *       Filename:  TcpConnectionBuffer.cc
 *        Created:  04/05/15 12:28:24
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <alpha/TcpConnectionBuffer.h>
#include <cstring>
#include <alpha/Compiler.h>
#include <alpha/Logger.h>

namespace alpha {

const size_t TcpConnectionBuffer::kDefaultBufferSize = 1 << 16;
const size_t TcpConnectionBuffer::kMaxBufferSize = 1 << 20;

TcpConnectionBuffer::TcpConnectionBuffer(size_t default_size)
    : read_index_(0), write_index_(0), internal_buffer_(default_size) {
  CHECK(default_size <= kMaxBufferSize);
}

TcpConnectionBuffer::~TcpConnectionBuffer() = default;

size_t TcpConnectionBuffer::max_size() const { return kMaxBufferSize; }

size_t TcpConnectionBuffer::capacity() const { return internal_buffer_.size(); }

size_t TcpConnectionBuffer::GetContiguousSpace() const {
  CheckIndex();
  return capacity() - write_index_;
}

char* TcpConnectionBuffer::WriteBegin() {
  CheckIndex();
  return &internal_buffer_[write_index_];
}

bool TcpConnectionBuffer::AddBytes(size_t n) {
  if (write_index_ + n > internal_buffer_.size()) {
    return false;
  };
  write_index_ += n;
  CheckIndex();
  return true;
}

bool TcpConnectionBuffer::Append(alpha::Slice s) {
  return Append(s.data(), s.size());
}

bool TcpConnectionBuffer::Append(const void* data, size_t size) {
  if (unlikely(!EnsureSpace(size))) {
    return false;
  }
  DCHECK(GetContiguousSpace() >= size);
  ::memcpy(WriteBegin(), data, size);
  AddBytes(size);
  return true;
}

size_t TcpConnectionBuffer::SpaceBeforeFull() const {
  return kMaxBufferSize - internal_buffer_.size() + GetContiguousSpace();
}

size_t TcpConnectionBuffer::BytesToRead() const {
  CheckIndex();
  return write_index_ - read_index_;
}

char* TcpConnectionBuffer::Read(size_t* length) {
  return const_cast<char*>(
      static_cast<const TcpConnectionBuffer*>(this)->Read(length));
}

const char* TcpConnectionBuffer::Read(size_t* length) const {
  CheckIndex();
  *length = BytesToRead();
  return *length ? &internal_buffer_[read_index_] : nullptr;
}

alpha::Slice TcpConnectionBuffer::Read() const {
  CheckIndex();
  return alpha::Slice(&internal_buffer_[read_index_],
                      write_index_ - read_index_);
}

size_t TcpConnectionBuffer::ReadAndClear(void* buf, size_t len) {
  size_t length;
  auto p = Read(&length);
  size_t n = std::min(len, length);
  ::memcpy(buf, p, n);
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
  CHECK(read_index_ <= write_index_);
  CHECK(write_index_ <= internal_buffer_.size());
}
}
