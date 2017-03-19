/*
 * =============================================================================
 *
 *       Filename:  MethodArgTypes.cc
 *        Created:  11/03/15 14:58:26
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "MethodArgTypes.h"
#include <cstring>
#include <alpha/Logger.h>

namespace amqp {

ShortString::ShortString() : size_(0) {}

ShortString::ShortString(alpha::Slice s) : size_(0) {
  append(s.data(), s.size());
}

ShortString& ShortString::operator=(alpha::Slice s) {
  clear();
  append(s.data(), s.size());
  return *this;
}

void ShortString::clear() { size_ = 0; }

void ShortString::append(const char* data, size_t size) {
  CHECK(size_ + size < kBufferSize) << "ShortString overflow";
  memcpy(buf_ + size_, data, size);
  size_ += size;
  buf_[size_] = '\0';
}

size_t ShortString::size() const { return size_; }

const char* ShortString::data() const { return buf_; }

std::string ShortString::str() const { return std::string(buf_, size_); }
}
