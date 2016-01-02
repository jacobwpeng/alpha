/*
 * =============================================================================
 *
 *       Filename:  IOBuffer.h
 *        Created:  12/29/15 10:19:14
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include <cstddef>

namespace alpha {
class IOBuffer {
 public:
  IOBuffer();
  virtual ~IOBuffer();
  explicit IOBuffer(size_t size);
  char* data() { return data_; }

  IOBuffer(const IOBuffer& other) = delete;
  IOBuffer& operator=(const IOBuffer& other) = delete;

 protected:
  explicit IOBuffer(char* data);

 private:
  char* data_;
};

class IOBufferWithSize : public IOBuffer {
 public:
  explicit IOBufferWithSize(size_t size);
  virtual ~IOBufferWithSize() override;
  size_t size() const { return size_; }

 protected:
  IOBufferWithSize(char* data, size_t size);

  size_t size_;
};

#if 0
class GrowableIOBuffer : public IOBuffer {
 public:
  GrowableIOBuffer();

  void set_offset(size_t offset);
  size_t offset() const { return offset_; }

  void SetCapacity(size_t capacity);
  size_t capacity() const { return capacity_; }

 private:
  ~GrowableIOBuffer() override;
  char* real_data_;
  size_t capacity_;
  size_t offset_;
};
#endif
}
