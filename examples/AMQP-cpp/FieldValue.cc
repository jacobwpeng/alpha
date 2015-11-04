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
#include "MethodArgTypes.h"
#include "FieldTable.h"

namespace detail {
  template<typename T>
  void CommonDestory(void* p) {
    delete reinterpret_cast<T*>(p);
  }

  template<typename T>
  void* CommonCopy(void* p) {
    return new T(*reinterpret_cast<T*>(p));
  }
}

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
  FieldValueAs(float, Type::kFloat, f);
  FieldValueAs(double, Type::kDouble, d);
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
  FieldValueAsPtr(BOOST_PP_EMPTY, ShortString, Type::kShortString);
  FieldValueAsPtr(const BOOST_PP_EMPTY, ShortString, Type::kShortString);
  FieldValueAsPtr(BOOST_PP_EMPTY, std::string, Type::kLongString);
  FieldValueAsPtr(const BOOST_PP_EMPTY, std::string, Type::kLongString);
#undef FieldValueAsPtr

  FieldValue::FieldValue(FieldValue::Type string_type, alpha::Slice arg) {
    CHECK(string_type == Type::kShortString
        || string_type == Type::kLongString)
      << "Invalid string_type" << string_type;
    type_ = string_type;
    if (type_ == Type::kLongString) {
      custom.ptr = new std::string(std::move(arg.ToString()));
      custom.destory_func = &detail::CommonDestory<std::string>;
      custom.copy_func = &detail::CommonCopy<std::string>;
    } else {
      custom.ptr = new ShortString(arg);
      custom.destory_func = &detail::CommonDestory<ShortString>;
      custom.copy_func = &detail::CommonCopy<ShortString>;
    }
  }

  FieldValue::FieldValue(const FieldTable& arg)
    : FieldValue(Type::kFieldTable,
        new FieldTable(arg),
        &detail::CommonDestory<FieldTable>,
        &detail::CommonCopy<FieldTable>) {
  }

  FieldValue::~FieldValue() {
    if (is_custom_type_value()) {
      custom.destory_func(custom.ptr);
    }
  }

  FieldValue::FieldValue(const FieldValue& other)
    :type_(Type::kEmpty) {
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

  std::ostream& operator<< (std::ostream& os, const FieldValue& v) {
    switch (v.type()) {
      case FieldValue::Type::kBoolean:
        os << (v.As<bool>() ? "true" : "false");
        break;
      case FieldValue::Type::kShortShortInt:
        os << v.As<int8_t>();
        break;
      case FieldValue::Type::kShortShortUInt:
        os << v.As<uint8_t>();
        break;
      case FieldValue::Type::kShortInt:
        os << v.As<int16_t>();
        break;
      case FieldValue::Type::kShortUInt:
        os << v.As<uint16_t>();
        break;
      case FieldValue::Type::kLongInt:
        os << v.As<int32_t>();
        break;
      case FieldValue::Type::kLongUInt:
        os << v.As<uint32_t>();
        break;
      case FieldValue::Type::kLongLongInt:
        os << v.As<int64_t>();
        break;
      case FieldValue::Type::kTimestamp:
      case FieldValue::Type::kLongLongUInt:
        os << v.As<uint64_t>();
        break;
      case FieldValue::Type::kFloat:
        os << v.As<float>();
        break;
      case FieldValue::Type::kDouble:
        os << v.As<double>();
        break;
      case FieldValue::Type::kShortString:
      case FieldValue::Type::kLongString:
        os << *v.AsPtr<std::string>();
        break;
      case FieldValue::Type::kFieldArray:
        os << "FieldValue-Array";
        break;
      case FieldValue::Type::kFieldTable:
        os << "FieldValue-Table";
        break;
      case FieldValue::Type::kDecimal:
        os << "FieldValue-Decimal";
        break;
      case FieldValue::Type::kEmpty:
        os << "FieldValue-Empty";
        break;
      default:
        os << "FieldValue-Unknown";
        break;
    }
    return os;
  }
}
