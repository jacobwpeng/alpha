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
#include <alpha/logger.h>
#include <alpha/format.h>
#include "CodedInputStream.h"

namespace amqp {
  OctetDecodeUnit::OctetDecodeUnit(uint8_t* res)
    :res_(res) {
  }

  int OctetDecodeUnit::ProcessMore(alpha::Slice& data) {
    CodedInputStream stream(data);
    bool ok = stream.ReadUInt8(res_);
    data.Advance(stream.consumed_bytes());
    return ok ? DecodeState::kDone : DecodeState::kNeedsMore;
  }

  ShortDecodeUnit::ShortDecodeUnit(uint16_t* res)
    :res_(res) {
  }

  int ShortDecodeUnit::ProcessMore(alpha::Slice& data) {
    CodedInputStream stream(data);
    bool ok = stream.ReadBigEndianUInt16(res_);
    data.Advance(stream.consumed_bytes());
    return ok ? DecodeState::kDone : DecodeState::kNeedsMore;
  }

  LongDecodeUnit::LongDecodeUnit(uint32_t* res)
    :res_(res) {
  }

  int LongDecodeUnit::ProcessMore(alpha::Slice& data) {
    CodedInputStream stream(data);
    bool ok = stream.ReadBigEndianUInt32(res_);
    data.Advance(stream.consumed_bytes());
    return ok ? DecodeState::kDone : DecodeState::kNeedsMore;
  }

  LongLongDecodeUnit::LongLongDecodeUnit(uint64_t* res)
    :res_(res) {
  }

  int LongLongDecodeUnit::ProcessMore(alpha::Slice& data) {
    CodedInputStream stream(data);
    bool ok = stream.ReadBigEndianUInt64(res_);
    data.Advance(stream.consumed_bytes());
    return ok ? DecodeState::kDone : DecodeState::kNeedsMore;
  }

  ShortStringDecodeUnit::ShortStringDecodeUnit(std::string* res)
    :res_(res), read_size_(false), sz_(0) {
  }

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
    :res_(res), read_size_(false), sz_(0) {
  }

  int LongStringDecodeUnit::ProcessMore(alpha::Slice& data) {
    CodedInputStream stream(data);
    if (!read_size_ && !stream.ReadBigEndianUInt32(&sz_)) {
      return DecodeState::kNeedsMore;
    }
    DLOG_INFO << "LongString expected size = " << sz_
      << ", res_->size() = " << res_->size()
      << ", data.size() = " << data.size();
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

  TimeStampDecodeUnit::TimeStampDecodeUnit(TimeStampDecodeUnit::ResultType res)
    :res_(res) {
  }

  int TimeStampDecodeUnit::ProcessMore(alpha::Slice& data) {
    uint64_t res;
    CodedInputStream stream(data);
    bool ok = stream.ReadBigEndianUInt64(&res);
    data.Advance(stream.consumed_bytes());
    return ok ? (*res_ = res, DecodeState::kDone) : DecodeState::kNeedsMore;
  }

  FieldTableDecodeUnit::FieldTableDecodeUnit(
      FieldTableDecodeUnit::ResultType res)
    :res_(res), underlying_decode_unit_(&raw_) {
  }

  int FieldTableDecodeUnit::ProcessMore(alpha::Slice& data) {
    DLOG_INFO << '\n' << alpha::HexDump(data);
    int rc = underlying_decode_unit_.ProcessMore(data);
    if (rc != 0) {
      return rc;
    }
    DLOG_INFO << "FieldTable LongString size = " << raw_.size();
    alpha::Slice table_raw(raw_);
    while (!table_raw.empty()) {
      std::string key, value;
      ShortStringDecodeUnit short_decode_unit(&key);
      rc = short_decode_unit.ProcessMore(table_raw);
      if (rc != 0) {
        LOG_WARNING << "Decode FieldTable key failed, rc = " << rc;
        return rc;
      }
      DLOG_INFO << "key: " << key;
      uint8_t field_value_type;
      OctetDecodeUnit field_value_type_decode_unit(&field_value_type);
      rc = field_value_type_decode_unit.ProcessMore(table_raw);
      if (rc != 0) {
        LOG_WARNING << "Decode FieldTable value type failed, rc = " << rc;
        return rc;
      }
      DLOG_INFO << "value type: " << static_cast<int>(field_value_type);
      LongStringDecodeUnit long_decode_unit(&value);
      rc = long_decode_unit.ProcessMore(table_raw);
      if (rc != 0) {
        LOG_WARNING << "Decode FieldTable value failed, rc = " << rc;
        return rc;
      }
      DLOG_INFO << "Key = " << key << ", value = " << value;
      res_->emplace(std::piecewise_construct,
          std::forward_as_tuple(key),
          std::forward_as_tuple(value));
    }
    return DecodeState::kDone;
  }
}
