/*
 * =============================================================================
 *
 *       Filename:  CodedInputStream.h
 *        Created:  10/21/15 10:35:32
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __CODEDINPUTSTREAM_H__
#define __CODEDINPUTSTREAM_H__

#include <alpha/slice.h>
#include "MethodArgTypes.h"

namespace amqp {

class CodedInputStream {
 public:
  CodedInputStream(alpha::Slice data);

  bool Skip(int amount);
  bool ReadPartialString(ShortString* out, int sz);
  bool ReadPartialString(std::string* out, int sz);
  bool ReadUInt8(uint8_t* val);
  bool ReadBigEndianUInt16(uint16_t* val);
  bool ReadBigEndianUInt32(uint32_t* val);
  bool ReadBigEndianUInt64(uint64_t* val);
  int consumed_bytes() const;

 private:
  bool Overflow(int amount) const;
  void Advance(int amount);
  const char* Ptr() const;

  int consumed_bytes_;
  alpha::Slice data_;
};
}

#endif /* ----- #ifndef __CODEDINPUTSTREAM_H__  ----- */
