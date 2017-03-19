/*
 * =============================================================================
 *
 *       Filename:  CodedInputStream.h
 *        Created:  12/11/15 16:24:08
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  Helper class for read from / write to binary stream
 *
 * =============================================================================
 */

#pragma once

#include <alpha/Slice.h>

namespace alpha {
class CodedInputStream {
 public:
  CodedInputStream(alpha::Slice data);

  bool Skip(int amount);
  bool ReadPartialString(std::string* out, int sz);
  bool ReadUInt8(uint8_t* val);
  bool ReadBigEndianUInt16(uint16_t* val);
  bool ReadBigEndianUInt32(uint32_t* val);
  bool ReadBigEndianUInt64(uint64_t* val);
  bool ReadLittleEndianUInt16(uint16_t* val);
  bool ReadLittleEndianUInt32(uint32_t* val);
  bool ReadLittleEndianUInt64(uint64_t* val);
  int consumed_bytes() const;

 private:
  bool Overflow(int amount) const;
  void Advance(int amount);
  const char* Ptr() const;

  int consumed_bytes_;
  alpha::Slice data_;
};
}

