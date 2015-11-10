/*
 * =============================================================================
 *
 *       Filename:  CodedInputStream.cc
 *        Created:  10/21/15 10:41:21
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "CodedInputStream.h"
#include <arpa/inet.h>
#include <alpha/logger.h>

namespace amqp {
CodedInputStream::CodedInputStream(alpha::Slice data)
    : consumed_bytes_(0), data_(data) {}

bool CodedInputStream::Skip(int amount) {
  if (Overflow(amount)) {
    return false;
  }
  Advance(amount);
  return true;
}

bool CodedInputStream::ReadPartialString(ShortString* out, int sz) {
  auto vsize = std::min<int>(sz, data_.size() - consumed_bytes_);
  out->append(Ptr(), vsize);
  Advance(vsize);
  return sz == vsize;
}

bool CodedInputStream::ReadPartialString(std::string* out, int sz) {
  auto vsize = std::min<int>(sz, data_.size() - consumed_bytes_);
  out->append(Ptr(), vsize);
  Advance(vsize);
  return sz == vsize;
}

bool CodedInputStream::ReadUInt8(uint8_t* val) {
  const int sz = sizeof(uint8_t);
  if (Overflow(sz)) {
    return false;
  }
  *val = *reinterpret_cast<const uint8_t*>(Ptr());
  Advance(sz);
  return true;
}

bool CodedInputStream::ReadBigEndianUInt16(uint16_t* val) {
  const int sz = sizeof(uint16_t);
  if (Overflow(sz)) {
    return false;
  }
  *val = *reinterpret_cast<const uint16_t*>(Ptr());
  *val = ntohs(*val);
  Advance(sz);
  return true;
}

bool CodedInputStream::ReadBigEndianUInt32(uint32_t* val) {
  const int sz = sizeof(uint32_t);
  if (Overflow(sz)) {
    return false;
  }
  *val = *reinterpret_cast<const uint32_t*>(Ptr());
  *val = ntohl(*val);
  Advance(sz);
  return true;
}

bool CodedInputStream::ReadBigEndianUInt64(uint64_t* val) {
  const int sz = sizeof(uint64_t);
  if (Overflow(sz)) {
    return false;
  }
  *val = *reinterpret_cast<const uint64_t*>(Ptr());
  *val = (((uint64_t)ntohl(*val)) << 32) + ntohl(*val >> 32);
  Advance(sz);
  return true;
}

int CodedInputStream::consumed_bytes() const { return consumed_bytes_; }

bool CodedInputStream::Overflow(int amount) const {
  return consumed_bytes_ + amount > static_cast<int>(data_.size());
}

void CodedInputStream::Advance(int amount) {
  CHECK(!Overflow(amount)) << "data overflow";
  consumed_bytes_ += amount;
}

const char* CodedInputStream::Ptr() const {
  return data_.data() + consumed_bytes_;
}
}
