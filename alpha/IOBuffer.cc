/*
 * =============================================================================
 *
 *       Filename:  IOBuffer.cc
 *        Created:  12/29/15 10:23:45
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <alpha/IOBuffer.h>
#include <stdlib.h>
#include <alpha/Logger.h>

namespace alpha {
IOBuffer::IOBuffer() : data_(nullptr) {}

IOBuffer::IOBuffer(size_t size) { data_ = (char*)malloc(size * sizeof(char)); }

IOBuffer::IOBuffer(char* data) : data_(data) {}

IOBuffer::~IOBuffer() { free(data_); }

IOBufferWithSize::IOBufferWithSize(size_t size) : IOBuffer(size), size_(size) {}

IOBufferWithSize::IOBufferWithSize(char* data, size_t size)
    : IOBuffer(data), size_(size) {}

IOBufferWithSize::~IOBufferWithSize() = default;

WrappedIOBuffer::WrappedIOBuffer(const void* data) : IOBuffer((char*)data) {}

WrappedIOBuffer::~WrappedIOBuffer() { data_ = nullptr; }

GrowableIOBuffer::GrowableIOBuffer()
    : IOBuffer(), real_data_(nullptr), capacity_(0), offset_(0) {}

GrowableIOBuffer::~GrowableIOBuffer() {
  free(real_data_);
  data_ = nullptr;
}

void GrowableIOBuffer::set_offset(size_t offset) {
  CHECK(offset <= capacity_);
  offset_ = offset;
  data_ = real_data_ + offset;
}

size_t GrowableIOBuffer::offset() const { return offset_; }

void GrowableIOBuffer::set_capacity(size_t capacity) {
  real_data_ = (char*)realloc(data_, capacity);
  if (offset_ >= capacity) {
    set_offset(capacity);
  } else {
    set_offset(offset_);
  }
}

size_t GrowableIOBuffer::capacity() const { return capacity_; }

char* GrowableIOBuffer::StartOfBuffer() const { return real_data_; }
}
