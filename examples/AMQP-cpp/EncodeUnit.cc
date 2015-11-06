/*
 * =============================================================================
 *
 *       Filename:  EncodeUnit.cc
 *        Created:  11/05/15 09:48:36
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include "EncodeUnit.h"
#include <limits>
#include <alpha/logger.h>
#include "CodedWriter.h"
#include "CodedOutputStream.h"

namespace amqp {

bool OctetEncodeUnit::Write(CodedWriterBase* w) {
  return CodedOutputStream(w).WriteUInt8(val_);
}

bool ShortEncodeUnit::Write(CodedWriterBase* w) {
  return CodedOutputStream(w).WriteBigEndianUInt16(val_);
}

bool LongEncodeUnit::Write(CodedWriterBase* w) {
  return CodedOutputStream(w).WriteBigEndianUInt32(val_);
}

bool LongLongEncodeUnit::Write(CodedWriterBase* w) {
  return CodedOutputStream(w).WriteBigEndianUInt64(val_);
}

ShortStringEncodeUnit::ShortStringEncodeUnit(const ShortString& s)
  :size_done_(false), consumed_(0), s_(s) {
}

bool ShortStringEncodeUnit::Write(CodedWriterBase* w) {
  CHECK(s_.size() <= std::numeric_limits<uint8_t>::max())
    << "Invalid ShortString size";
  const uint8_t sz = s_.size();
  if (!size_done_) {
    size_done_ = OctetEncodeUnit(sz).Write(w);
    if (!size_done_) return false;
  }
  CHECK(consumed_ <= sz) << "Invalid consumed bytes";
  if (consumed_ == sz) return true;

  auto pos = s_.data() + consumed_;
  auto left = sz - consumed_;
  consumed_ += CodedOutputStream(w).WriteBinary(pos, left);
  return consumed_ == sz;
}

LongStringEncodeUnit::LongStringEncodeUnit(const LongString& s)
  :size_done_(false), consumed_(0), s_(s) {
}

bool LongStringEncodeUnit::Write(CodedWriterBase* w) {
  CHECK(s_.size() <= std::numeric_limits<uint32_t>::max())
    << "Invalid LongString size";
  const uint32_t sz = s_.size();
  if (!size_done_) {
    size_done_ = LongEncodeUnit(sz).Write(w);
    if (!size_done_) return false;
  }
  CHECK(consumed_ <= sz) << "Invalid consumed bytes";
  if (consumed_ == sz) return true;

  auto pos = s_.data() + consumed_;
  auto left = sz - consumed_;
  consumed_ += CodedOutputStream(w).WriteBinary(pos, left);
  return consumed_ == sz;
}

FieldValueEncodeUnit::FieldValueEncodeUnit(const FieldValue& v)
  :val_(v) {
    switch (val_.type()) {
      case FieldValue::Type::kShortShortInt:
        underlying_encode_unit_.reset(new OctetEncodeUnit(
              val_.As<int8_t>()));
        break;
      case FieldValue::Type::kShortShortUInt:
        underlying_encode_unit_.reset(new OctetEncodeUnit(
              val_.As<uint8_t>()));
        break;
      case FieldValue::Type::kShortInt:
        underlying_encode_unit_.reset(new ShortEncodeUnit(
              val_.As<int16_t>()));
        break;
      case FieldValue::Type::kShortUInt:
        underlying_encode_unit_.reset(new ShortEncodeUnit(
              val_.As<uint16_t>()));
        break;
      case FieldValue::Type::kLongInt:
        underlying_encode_unit_.reset(new LongEncodeUnit(
              val_.As<int32_t>()));
        break;
      case FieldValue::Type::kLongUInt:
        underlying_encode_unit_.reset(new LongEncodeUnit(
              val_.As<uint32_t>()));
        break;
      case FieldValue::Type::kLongLongInt:
        underlying_encode_unit_.reset(new LongLongEncodeUnit(
              val_.As<int64_t>()));
        break;
      case FieldValue::Type::kLongLongUInt:
        underlying_encode_unit_.reset(new LongLongEncodeUnit(
              val_.As<uint64_t>()));
        break;
      case FieldValue::Type::kShortString:
        underlying_encode_unit_.reset(new ShortStringEncodeUnit(
            *val_.AsPtr<ShortString>()));
        break;
      case FieldValue::Type::kLongString:
        underlying_encode_unit_.reset(new LongStringEncodeUnit(
            *val_.AsPtr<LongString>()));
        break;
      default:
        CHECK(false) << "Unsupported FieldValue::Type ";
    }
}

bool FieldValueEncodeUnit::Write(CodedWriterBase* w) {
  return underlying_encode_unit_->Write(w);
}

FieldTableEncodeUnit::FieldTableEncodeUnit(const FieldTable& ft)
  :ft_(ft) {
}

bool FieldTableEncodeUnit::Write(CodedWriterBase* w) {
  if (encoded_.empty()) {
    MemoryStringWriter w(&encoded_);
    // Write to In-memory string first
    for (const auto& p : ft_.underlying_map()) {
      ShortStringEncodeUnit key_encode_unit(ShortString(p.first));
      key_encode_unit.Write(&w);
      OctetEncodeUnit value_type_encode_unit(
          static_cast<uint8_t>(p.second.type()));
      value_type_encode_unit.Write(&w);
      FieldValueEncodeUnit value_encode_unit(p.second);
      value_encode_unit.Write(&w);
    }
  }

  if (!long_string_encode_unit_) {
    long_string_encode_unit_.reset (new LongStringEncodeUnit(encoded_));
  }

  // Then write as LongString
  return long_string_encode_unit_->Write(w);
}
}
