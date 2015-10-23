/*
 * =============================================================================
 *
 *       Filename:  FieldValue.cc
 *        Created:  10/23/15 15:08:51
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include "FieldValue.h"
#include <cstring>
#include <boost/preprocessor/facilities/empty.hpp>
#include <alpha/logger.h>
#include "FieldTable.h"

namespace amqp {
#define FieldValueAs(cpp_type, type_enum, field)                     \
  template<>                                                         \
  cpp_type FieldValue::As<cpp_type> () const {                       \
    CHECK(type_ == type_enum) << "Expected type: `" << type_enum     \
      << "', Actual type: `" << type_ << "'";                        \
    return field;                                                    \
  }

  FieldValueAs(bool, Type::kBoolean, boolean);
  FieldValueAs(int8_t, Type::kShortShortInt, int8);
  FieldValueAs(uint8_t, Type::kShortShortUInt, uint8);
  FieldValueAs(int16_t, Type::kShortInt, int16);
  FieldValueAs(uint16_t, Type::kShortUInt, uint16);
  FieldValueAs(int32_t, Type::kLongInt, int32);
  FieldValueAs(uint32_t, Type::kLongUInt, uint32);
  FieldValueAs(int64_t, Type::kLongLongInt, int64);
  FieldValueAs(uint64_t, Type::kLongLongUInt, uint64);
#undef FieldValueAs

#define FieldValueAsPtr(C, cpp_type, type_enum)                      \
  template<>                                                         \
  typename std::add_pointer<C() cpp_type>::type                      \
  FieldValue::AsPtr<cpp_type> () C() {                               \
    CHECK(type_ == type_enum) << "Expected type: `" << type_enum     \
      << "', Actual type: `" << type_ << "'";                        \
    return reinterpret_cast<                                         \
            C() typename std::add_pointer<cpp_type>::type            \
          >(custom.ptr);                                             \
  }

  FieldValueAsPtr(BOOST_PP_EMPTY, FieldTable, Type::kFieldTable);
  FieldValueAsPtr(const BOOST_PP_EMPTY, FieldTable, Type::kFieldTable);
#undef FieldValueAsPtr

  template<>
  std::string* FieldValue::AsPtr<std::string>() {
    CHECK(type_ == Type::kShortString || type_ == Type::kLongString)
      << "Expecetd type: `s' or `S', Actual type: `" << type_ << "'";
    return reinterpret_cast<std::string*>(custom.ptr);
  }

  template<>
  const std::string* FieldValue::AsPtr<std::string>() const {
    CHECK(type_ == Type::kShortString || type_ == Type::kLongString)
      << "Expecetd type: `s' or `S', Actual type: `" << type_ << "'";
    return reinterpret_cast<const std::string*>(custom.ptr);
  }

  void FieldValue::DestoryString(void* p) {
    delete reinterpret_cast<std::string*>(p);
  }

  void FieldValue::DestoryFieldTable(void* p) {
    delete reinterpret_cast<FieldTable*>(p);
  }

  void* FieldValue::CopyString(void* p) {
    return new std::string(*reinterpret_cast<std::string*>(p));
  }

  void* FieldValue::CopyFieldTable(void* p) {
    return new FieldTable(*reinterpret_cast<FieldTable*>(p));
  }

  FieldValue::FieldValue(FieldValue::Type string_type, alpha::Slice arg) {
    CHECK(string_type == Type::kShortString
        || string_type == Type::kLongString)
      << "Invalid string_type" << string_type;
    type_ = string_type;
    custom.ptr = new std::string(std::move(arg.ToString()));
    custom.destory_func = &FieldValue::DestoryString;
    custom.copy_func = &FieldValue::CopyString;
  }

  FieldValue::FieldValue(const FieldTable& arg)
    : FieldValue(Type::kFieldTable,
        new FieldTable(arg),
        &FieldValue::DestoryFieldTable,
        &FieldValue::CopyFieldTable) {
  }

  FieldValue::~FieldValue() {
    if (is_custom_type_value()) {
      custom.destory_func(custom.ptr);
    }
  }

  FieldValue::FieldValue(const FieldValue& other) {
    *this = other;
  }

  FieldValue& FieldValue::operator= (const FieldValue& other) {
    if (this != &other) {
      if (is_custom_type_value()) {
        custom.destory_func(custom.ptr);
      }
      type_ = other.type_;
      custom = other.custom;
      if (is_custom_type_value()) {
        custom.ptr = custom.copy_func(other.custom.ptr);
      }
    }
    return *this;
  }

  FieldValue::Type FieldValue::type() const {
    return type_;
  }

  bool FieldValue::is_custom_type_value() const {
    switch (type_) {
      case Type::kShortString:
      case Type::kLongString:
      case Type::kFieldArray:
      case Type::kFieldTable:
      case Type::kDecimal:
        return true;
      default:
        return false;
    }
  }
}
