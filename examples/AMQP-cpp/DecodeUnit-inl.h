/*
 * =============================================================================
 *
 *       Filename:  DecodeUnit-inl.h
 *        Created:  11/02/15 16:26:19
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __DECODEUNIT_H__
#error This file may only be included from DecodeUnit.h.
#endif

#include <alpha/Compiler.h>
#include <alpha/Logger.h>

namespace amqp {

template <typename UnderlyingDecodeUnitType,
          typename ValueCppType,
          typename DecodeUnitArgType>
std::unique_ptr<FieldValueDecodeUnit> FieldValueDecodeUnit::Create() {
  auto res = alpha::make_unique<FieldValueDecodeUnit>();
  res->val_ = FieldValue(ValueCppType());
  res->underlying_decode_unit_ = alpha::make_unique<UnderlyingDecodeUnitType>(
      reinterpret_cast<DecodeUnitArgType*>(res->val_.Ptr()));
  return res;
}

template <typename UnderlyingDecodeUnitType, typename ValueCppType>
std::unique_ptr<FieldValueDecodeUnit> FieldValueDecodeUnit::Create(
    FieldValue::Type string_type) {
  auto res = alpha::make_unique<FieldValueDecodeUnit>();
  res->val_ = FieldValue(string_type, "");
  res->underlying_decode_unit_ = alpha::make_unique<UnderlyingDecodeUnitType>(
      res->val_.AsPtr<ValueCppType>());
  return res;
}

template <typename UnderlyingDecodeUnitType, typename ValueCppType>
std::unique_ptr<FieldValueDecodeUnit> FieldValueDecodeUnit::Create(
    const CodecEnv* env) {
  auto res = alpha::make_unique<FieldValueDecodeUnit>();
  res->val_ = FieldValue(ValueCppType());
  res->underlying_decode_unit_ = alpha::make_unique<UnderlyingDecodeUnitType>(
      res->val_.AsPtr<ValueCppType>(), env);
  return res;
}
}
