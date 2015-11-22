/*
 * =============================================================================
 *
 *       Filename:  MethodArgTypes.h
 *        Created:  11/02/15 12:46:27
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __METHODARGTYPES_H__
#define __METHODARGTYPES_H__

#include "FieldTable.h"

namespace amqp {
using ClassID = uint16_t;
using MethodID = uint16_t;
using Timestamp = uint64_t;
using LongString = std::string;

using ChannelID = uint16_t;

class ShortString {
 public:
  ShortString();
  ShortString(alpha::Slice s);
  ShortString& operator=(alpha::Slice s);
  void clear();
  void append(const char* data, size_t sz);
  size_t size() const;
  const char* data() const;
  std::string str() const;

 private:
  static const size_t kBufferSize = 256;
  size_t size_;
  char buf_[kBufferSize];
};
}

#endif /* ----- #ifndef __METHODARGTYPES_H__  ----- */
