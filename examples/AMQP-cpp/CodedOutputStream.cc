/*
 * =============================================================================
 *
 *       Filename:  CodedOutputStream.cc
 *        Created:  11/04/15 16:39:03
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "CodedOutputStream.h"
#include <alpha/Endian.h>
#include "CodedWriter.h"

namespace amqp {

CodedOutputStream::CodedOutputStream(CodedWriterBase* w) : w_(w) {}

bool CodedOutputStream::WriteUInt8(uint8_t val) {
  return w_->CanWrite(1) && w_->Write(&val, 1);
}

bool CodedOutputStream::WriteBigEndianUInt16(uint16_t val) {
  if (w_->CanWrite(2)) {
    auto v = alpha::HostToBigEndian(val);
    return w_->Write(&v, 2);
  }
  return false;
}

bool CodedOutputStream::WriteBigEndianUInt32(uint32_t val) {
  if (w_->CanWrite(4)) {
    auto v = alpha::HostToBigEndian(val);
    return w_->Write(&v, 4);
  }
  return false;
}

bool CodedOutputStream::WriteBigEndianUInt64(uint64_t val) {
  if (w_->CanWrite(8)) {
    auto v = alpha::HostToBigEndian(val);
    return w_->Write(&v, 8);
  }
  return false;
}

size_t CodedOutputStream::WriteBinary(const char* buf, size_t sz) {
  return w_->Write(buf, sz);
}
}
