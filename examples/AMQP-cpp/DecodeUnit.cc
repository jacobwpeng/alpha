/*
 * =============================================================================
 *
 *       Filename:  DecodeUnit.cc
 *        Created:  10/21/15 10:27:04
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "DecodeUnit.h"
#include <alpha/Logger.h>
#include <alpha/Format.h>
#include "CodecEnv.h"
#include <alpha/CodedInputStream.h>

namespace amqp {
BooleanDecodeUnit::BooleanDecodeUnit(bool* res) : res_(res) {}

int BooleanDecodeUnit::ProcessMore(alpha::Slice& data) {
  uint8_t u8;
  CodedInputStream stream(data);
  bool ok = stream.ReadUInt8(&u8);
  if (ok) {
    data.Advance(stream.consumed_bytes());
    *res_ = u8;
    return DecodeState::kDone;
  }
  return DecodeState::kNeedsMore;
}

OctetDecodeUnit::OctetDecodeUnit(uint8_t* res) : res_(res) {}

int OctetDecodeUnit::ProcessMore(alpha::Slice& data) {
  CodedInputStream stream(data);
  bool ok = stream.ReadUInt8(res_);
  data.Advance(stream.consumed_bytes());
  return ok ? DecodeState::kDone : DecodeState::kNeedsMore;
}

ShortDecodeUnit::ShortDecodeUnit(uint16_t* res) : res_(res) {}

int ShortDecodeUnit::ProcessMore(alpha::Slice& data) {
  CodedInputStream stream(data);
  bool ok = stream.ReadBigEndianUInt16(res_);
  data.Advance(stream.consumed_bytes());
  return ok ? DecodeState::kDone : DecodeState::kNeedsMore;
}

LongDecodeUnit::LongDecodeUnit(uint32_t* res) : res_(res) {}

int LongDecodeUnit::ProcessMore(alpha::Slice& data) {
  CodedInputStream stream(data);
  bool ok = stream.ReadBigEndianUInt32(res_);
  data.Advance(stream.consumed_bytes());
  return ok ? DecodeState::kDone : DecodeState::kNeedsMore;
}

LongLongDecodeUnit::LongLongDecodeUnit(uint64_t* res) : res_(res) {}

int LongLongDecodeUnit::ProcessMore(alpha::Slice& data) {
  CodedInputStream stream(data);
  bool ok = stream.ReadBigEndianUInt64(res_);
  data.Advance(stream.consumed_bytes());
  return ok ? DecodeState::kDone : DecodeState::kNeedsMore;
}

ShortStringDecodeUnit::ShortStringDecodeUnit(ShortString* res)
    : res_(res), read_size_(false), sz_(0) {}

int ShortStringDecodeUnit::ProcessMore(alpha::Slice& data) {
  CodedInputStream stream(data);
  if (!read_size_ && !stream.ReadUInt8(&sz_)) {
    return DecodeState::kNeedsMore;
  }
  read_size_ = true;
  if (sz_ == 0) {
    data.Advance(stream.consumed_bytes());
    return DecodeState::kDone;
  }
  CHECK(res_->size() < sz_) << "Try to read more bytes from stream";
  bool ok = stream.ReadPartialString(res_, sz_ - res_->size());
  data.Advance(stream.consumed_bytes());
  return ok ? DecodeState::kDone : DecodeState::kNeedsMore;
}

LongStringDecodeUnit::LongStringDecodeUnit(std::string* res)
    : res_(res), read_size_(false), sz_(0) {}

int LongStringDecodeUnit::ProcessMore(alpha::Slice& data) {
  CodedInputStream stream(data);
  if (!read_size_ && !stream.ReadBigEndianUInt32(&sz_)) {
    return DecodeState::kNeedsMore;
  }
  // DLOG_INFO << "LongString expected size = " << sz_
  //  << ", res_->size() = " << res_->size()
  //  << ", data.size() = " << data.size();
  read_size_ = true;
  if (sz_ == 0) {
    data.Advance(stream.consumed_bytes());
    return DecodeState::kDone;
  }
  CHECK(res_->size() < sz_) << "Try to read more bytes from stream";
  bool ok = stream.ReadPartialString(res_, sz_ - res_->size());
  data.Advance(stream.consumed_bytes());
  return ok ? DecodeState::kDone : DecodeState::kNeedsMore;
}

TimeStampDecodeUnit::TimeStampDecodeUnit(Timestamp* res) : res_(res) {}

int TimeStampDecodeUnit::ProcessMore(alpha::Slice& data) {
  uint64_t res;
  CodedInputStream stream(data);
  bool ok = stream.ReadBigEndianUInt64(&res);
  data.Advance(stream.consumed_bytes());
  return ok ? (*res_ = res, DecodeState::kDone) : DecodeState::kNeedsMore;
}

FieldTableDecodeUnit::FieldTableDecodeUnit(FieldTableDecodeUnit::ResultType res,
                                           const CodecEnv* env)
    : env_(env), res_(res), underlying_decode_unit_(&raw_) {}

int FieldTableDecodeUnit::ProcessMore(alpha::Slice& data) {
  int rc = underlying_decode_unit_.ProcessMore(data);
  if (rc != 0) {
    return rc;
  }
  alpha::Slice table_data(raw_);
  while (!table_data.empty()) {
    ShortString key;
    ShortStringDecodeUnit key_decode_unit(&key);
    rc = key_decode_unit.ProcessMore(table_data);
    if (rc != 0) {
      LOG_WARNING << "Decode FieldTable key failed, rc = " << rc;
      return rc;
    }
    uint8_t value_type;
    OctetDecodeUnit value_type_decode_unit(&value_type);
    rc = value_type_decode_unit.ProcessMore(table_data);
    if (rc != 0) {
      LOG_WARNING << "Decode FieldTable value type failed, rc = " << rc;
      return rc;
    }
    auto value_decode_unit = env_->NewDecodeUnit(value_type);
    rc = value_decode_unit->ProcessMore(table_data);
    if (rc != 0) {
      LOG_WARNING << "Decode FieldTable value failed, value_type = "
                  << value_type;
      return rc;
    }
    auto p = res_->Insert(key.str(), value_decode_unit->Get());
    CHECK(p.second) << "Same key found in FieldTable";
    // DLOG_INFO << "Key: " << key << ", Value: " << *p.first;
  }
  return DecodeState::kDone;
}

const FieldValue& FieldValueDecodeUnit::Get() const { return val_; }

int FieldValueDecodeUnit::ProcessMore(alpha::Slice& data) {
  return underlying_decode_unit_->ProcessMore(data);
}
}
