/*
 * =============================================================================
 *
 *       Filename:  tcp_connection_buffer.h
 *        Created:  04/05/15 12:19:29
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include <vector>
#include <alpha/slice.h>

namespace alpha {
class TcpConnectionBuffer final {
 public:
  static const size_t kDefaultBufferSize;
  static const size_t kMaxBufferSize;

  TcpConnectionBuffer(size_t default_size = kDefaultBufferSize);
  ~TcpConnectionBuffer();

  size_t max_size() const;
  size_t capacity() const;

  //不触发扩容的写
  size_t GetContiguousSpace() const;
  char* WriteBegin();
  bool AddBytes(size_t n);
  bool EnsureSpace(size_t n);

  //写入(可能会扩容, 超限返回false)
  bool Append(alpha::Slice s);
  bool Append(const void* data, size_t size);
  size_t SpaceBeforeFull() const;

  size_t BytesToRead() const;
  char* Read(size_t* length);
  const char* Read(size_t* length) const;
  alpha::Slice Read() const;
  size_t ReadAndClear(void* buf, size_t len);
  void ConsumeBytes(size_t len);

 private:
  void CheckIndex() const;
  size_t read_index_;
  size_t write_index_;
  std::vector<char> internal_buffer_;
};
}
