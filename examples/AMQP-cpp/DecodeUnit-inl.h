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

#include <alpha/logger.h>

namespace amqp {

  template<typename UnderlyingDecodeUnitType,
           typename ValueCppType,
           typename DecodeUnitArgType>
  std::unique_ptr<FieldValueDecodeUnit> FieldValueDecodeUnit::Create() {
    std::unique_ptr<FieldValueDecodeUnit> res(new FieldValueDecodeUnit());
    res->val_ = FieldValue(ValueCppType());
    res->underlying_decode_unit_ = std::unique_ptr<UnderlyingDecodeUnitType>(
        new UnderlyingDecodeUnitType(
          reinterpret_cast<DecodeUnitArgType*>(res->val_.Ptr())
        )
    );
    return res;
  }

  template<typename UnderlyingDecodeUnitType,
           typename ValueCppType>
  std::unique_ptr<FieldValueDecodeUnit> FieldValueDecodeUnit::Create(
      FieldValue::Type string_type) {
    std::unique_ptr<FieldValueDecodeUnit> res(new FieldValueDecodeUnit());
    //DLOG_INFO << "Before copy FieldValue";
    res->val_ = FieldValue(string_type, "");
    //DLOG_INFO << "res->val_.AsPtr() = " << res->val_.AsPtr<std::string>();
    res->underlying_decode_unit_ = std::unique_ptr<UnderlyingDecodeUnitType>(
        new UnderlyingDecodeUnitType(res->val_.AsPtr<ValueCppType>()));
    return res;
  }

  template<typename UnderlyingDecodeUnitType,
           typename ValueCppType>
  std::unique_ptr<FieldValueDecodeUnit> FieldValueDecodeUnit::Create() {
    std::unique_ptr<FieldValueDecodeUnit> res(new FieldValueDecodeUnit());
    res->val_ = FieldValue(ValueCppType());
    res->underlying_decode_unit_ = std::unique_ptr<UnderlyingDecodeUnitType>(
        new UnderlyingDecodeUnitType(res->val_.AsPtr<ValueCppType>()));
    return res;
  }
}
