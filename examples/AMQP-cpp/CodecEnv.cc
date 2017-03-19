/*
 * =============================================================================
 *
 *       Filename:  CodecEnv.cc
 *        Created:  11/09/15 11:13:15
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "CodecEnv.h"

#include <cstring>
#include <algorithm>
#include <alpha/Compiler.h>
#include "DecodeUnit.h"
#include "FieldValue.h"

namespace amqp {
class RabbitMQCodecEnv final : public CodecEnv {
 public:
  // See https://www.rabbitmq.com/amqp-0-9-1-errata.html
  RabbitMQCodecEnv();
  virtual std::unique_ptr<FieldValueDecodeUnit> NewDecodeUnit(
      uint8_t type) const override;
  virtual uint8_t FieldValueType(const FieldValue& v) const override;
  virtual std::unique_ptr<EncodeUnit> NewEncodeUnit(
      const FieldValue& v) const override;
};
RabbitMQCodecEnv::RabbitMQCodecEnv() = default;

const CodecEnv* GetCodecEnv(alpha::Slice product) {
  static const RabbitMQCodecEnv rabbit;
  if (product.find("RabbitMQ") != alpha::Slice::npos) {
    return &rabbit;
  }
  return &rabbit;
}

std::unique_ptr<FieldValueDecodeUnit> RabbitMQCodecEnv::NewDecodeUnit(
    uint8_t type) const {
  switch (type) {
    case 't':
      return FieldValueDecodeUnit::Create<OctetDecodeUnit, bool, uint8_t>();
    case 'b':
      return FieldValueDecodeUnit::Create<OctetDecodeUnit, int8_t, uint8_t>();
    case 's':
      return FieldValueDecodeUnit::Create<ShortDecodeUnit, int16_t, uint16_t>();
    case 'I':
      return FieldValueDecodeUnit::Create<LongDecodeUnit, int32_t, uint32_t>();
    case 'l':
      return FieldValueDecodeUnit::Create<LongLongDecodeUnit,
                                          int64_t,
                                          uint64_t>();
    case 'T':
      return FieldValueDecodeUnit::Create<LongLongDecodeUnit,
                                          uint64_t,
                                          uint64_t>();
    case 'S':
      return FieldValueDecodeUnit::Create<LongStringDecodeUnit, std::string>(
          FieldValue::Type::kLongString);
    case 'F':
      return FieldValueDecodeUnit::Create<FieldTableDecodeUnit, FieldTable>(
          this);
    default:
      // TODO: throw a ConnectionExpcetion
      CHECK(false) << "Unknown value_type: `" << type << "'";
      return nullptr;
  }
}

uint8_t RabbitMQCodecEnv::FieldValueType(const FieldValue& v) const {
  switch (v.type()) {
    case FieldValue::Type::kBoolean:
      return 't';
    case FieldValue::Type::kShortShortInt:
    case FieldValue::Type::kShortShortUInt:
      return 'b';
    case FieldValue::Type::kShortInt:
    case FieldValue::Type::kShortUInt:
      return 's';
    case FieldValue::Type::kLongInt:
    case FieldValue::Type::kLongUInt:
      return 'I';
    case FieldValue::Type::kLongLongInt:
    case FieldValue::Type::kLongLongUInt:
      return 'l';
    case FieldValue::Type::kShortString:
    case FieldValue::Type::kLongString:
      return 'S';
    case FieldValue::Type::kFieldArray:
      return 'A';
    case FieldValue::Type::kTimestamp:
      return 'U';
    case FieldValue::Type::kFieldTable:
      return 'F';
    case FieldValue::Type::kEmpty:
      return 'V';
    default:
      // TODO: throw a ConnectionExpcetion
      CHECK(false) << "Unknown value_type: `" << v.type() << "'";
      return '\0';
  }
}

std::unique_ptr<EncodeUnit> RabbitMQCodecEnv::NewEncodeUnit(
    const FieldValue& v) const {
  switch (v.type()) {
    case FieldValue::Type::kShortShortInt:
      return alpha::make_unique<OctetEncodeUnit>(v.As<int8_t>());
    case FieldValue::Type::kShortShortUInt:
      return alpha::make_unique<OctetEncodeUnit>(v.As<uint8_t>());
    case FieldValue::Type::kShortInt:
      return alpha::make_unique<ShortEncodeUnit>(v.As<int16_t>());
    case FieldValue::Type::kShortUInt:
      return alpha::make_unique<ShortEncodeUnit>(v.As<uint16_t>());
    case FieldValue::Type::kLongInt:
      return alpha::make_unique<LongEncodeUnit>(v.As<int32_t>());
    case FieldValue::Type::kLongUInt:
      return alpha::make_unique<LongEncodeUnit>(v.As<uint32_t>());
    case FieldValue::Type::kLongLongInt:
      return alpha::make_unique<LongLongEncodeUnit>(v.As<int64_t>());
    case FieldValue::Type::kLongLongUInt:
      return alpha::make_unique<LongLongEncodeUnit>(v.As<uint64_t>());
    case FieldValue::Type::kShortString:
      return alpha::make_unique<LongStringEncodeUnit>(alpha::Slice(
          v.AsPtr<ShortString>()->data(), v.AsPtr<ShortString>()->size()));
    case FieldValue::Type::kLongString:
      return alpha::make_unique<LongStringEncodeUnit>(*v.AsPtr<std::string>());
    case FieldValue::Type::kFieldTable:
      return alpha::make_unique<FieldTableEncodeUnit>(*v.AsPtr<FieldTable>(),
                                                      this);
    default:
      // TODO: throw a ConnectionExpcetion
      CHECK(false) << "Unknown value_type: `" << v.type() << "'";
      return nullptr;
  }
}
}
