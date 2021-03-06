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
#include <alpha/Compiler.h>

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

class WrappedIOBuffer : public IOBuffer {
 public:
  explicit WrappedIOBuffer(const void* data);
  virtual ~WrappedIOBuffer() override;
};

class GrowableIOBuffer : public IOBuffer {
 public:
  GrowableIOBuffer();
  virtual ~GrowableIOBuffer() override;
  DISABLE_COPY_ASSIGNMENT(GrowableIOBuffer);

  void set_offset(size_t offset);
  size_t offset() const;

  void set_capacity(size_t capacity);
  size_t capacity() const;
  size_t RemainingCapacity() const;

  char* StartOfBuffer() const;

 private:
  char* real_data_;
  size_t capacity_;
  size_t offset_;
};
}
