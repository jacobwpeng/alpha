/*
 * =============================================================================
 *
 *       Filename:  CodedInputStream.cc
 *        Created:  12/11/15 16:26:21
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <alpha/CodedInputStream.h>
#include <alpha/Logger.h>
#include <alpha/Endian.h>

namespace alpha {
CodedInputStream::CodedInputStream(alpha::Slice data)
    : consumed_bytes_(0), data_(data) {}

bool CodedInputStream::Skip(int amount) {
  if (Overflow(amount)) {
    return false;
  }
  Advance(amount);
  return true;
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
  *val = alpha::BigEndianToHost(*val);
  Advance(sz);
  return true;
}

bool CodedInputStream::ReadBigEndianUInt32(uint32_t* val) {
  const int sz = sizeof(uint32_t);
  if (Overflow(sz)) {
    return false;
  }
  *val = *reinterpret_cast<const uint32_t*>(Ptr());
  *val = alpha::BigEndianToHost(*val);
  Advance(sz);
  return true;
}

bool CodedInputStream::ReadBigEndianUInt64(uint64_t* val) {
  const int sz = sizeof(uint64_t);
  if (Overflow(sz)) {
    return false;
  }
  *val = *reinterpret_cast<const uint64_t*>(Ptr());
  *val = alpha::BigEndianToHost(*val);
  Advance(sz);
  return true;
}

bool CodedInputStream::ReadLittleEndianUInt16(uint16_t* val) {
  const int sz = sizeof(uint16_t);
  if (Overflow(sz)) {
    return false;
  }
  *val = *reinterpret_cast<const uint16_t*>(Ptr());
  *val = alpha::LittleEndianToHost(*val);
  Advance(sz);
  return true;
}

bool CodedInputStream::ReadLittleEndianUInt32(uint32_t* val) {
  const int sz = sizeof(uint32_t);
  if (Overflow(sz)) {
    return false;
  }
  *val = *reinterpret_cast<const uint32_t*>(Ptr());
  *val = alpha::LittleEndianToHost(*val);
  Advance(sz);
  return true;
}

bool CodedInputStream::ReadLittleEndianUInt64(uint64_t* val) {
  const int sz = sizeof(uint64_t);
  if (Overflow(sz)) {
    return false;
  }
  *val = *reinterpret_cast<const uint64_t*>(Ptr());
  *val = alpha::LittleEndianToHost(*val);
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
