/*
 * =============================================================================
 *
 *       Filename:  CodedOutputStream.h
 *        Created:  11/04/15 16:27:58
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include <cstdint>

namespace amqp {

using std::size_t;
class CodedWriterBase;
class CodedOutputStream {
 public:
  CodedOutputStream(CodedWriterBase* w);

  bool WriteUInt8(uint8_t val);
  bool WriteBigEndianUInt16(uint16_t val);
  bool WriteBigEndianUInt32(uint32_t val);
  bool WriteBigEndianUInt64(uint64_t val);
  size_t WriteBinary(const char* buf, size_t sz);

 private:
  CodedWriterBase* w_;
};
}

