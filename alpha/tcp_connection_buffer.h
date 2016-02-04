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

#ifndef __TCP_CONNECTION_BUFFER_H__
#define __TCP_CONNECTION_BUFFER_H__

#include "slice.h"
#include <vector>

namespace alpha {
class TcpConnectionBuffer {
 public:
  TcpConnectionBuffer();
  ~TcpConnectionBuffer();

  //不触发扩容的写
  size_t GetContiguousSpace() const;
  char* WriteBegin();
  void AddBytes(size_t len);

  //写入(可能会扩容, 超限返回false)
  bool Append(const alpha::Slice& data);
  bool Append(const void* data, size_t size);
  size_t SpaceBeforeFull() const;

  char* Read(size_t* length);
  const char* Read(size_t* length) const;
  alpha::Slice Read() const;
  size_t ReadAndClear(void* buf, size_t len);
  void ConsumeBytes(size_t len);

 private:
  bool EnsureSpace(size_t n);
  void CheckIndex() const;

  static const size_t kDefaultBufferSize = 1 << 16;  // 64KiB
  static const size_t kMaxBufferSize;
  std::vector<char> internal_buffer_;
  size_t read_index_ = 0;
  size_t write_index_ = 0;
};
}

#endif /* ----- #ifndef __TCP_CONNECTION_BUFFER_H__  ----- */
