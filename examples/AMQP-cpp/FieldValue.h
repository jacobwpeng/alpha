/*
 * =============================================================================
 *
 *       Filename:  FieldValue.h
 *        Created:  10/23/15 11:13:26
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#ifndef  __FIELDVALUE_H__
#define  __FIELDVALUE_H__

#include <string>
#include <alpha/slice.h>

namespace amqp {

class FieldArray;
class FieldTable;

class FieldValue {
  public:
    enum class Type : uint8_t {
      kBoolean,
      kShortShortInt,
      kShortShortUInt,
      kShortInt,
      kShortUInt,
      kLongInt,
      kLongUInt,
      kLongLongInt,
      kLongLongUInt,
      kFloat,
      kDouble,
      kDecimal,
      kShortString,
      kLongString,
      kFieldArray,
      kTimestamp,
      kFieldTable,
      kEmpty
    };

  private:
    using CustomValueDestoryFunc = void (*)(void*);
    using CustomValueCopyFunc = void* (*)(void*);
    struct CustomValue {
      void* ptr = nullptr;
      CustomValueDestoryFunc destory_func = nullptr;
      CustomValueCopyFunc copy_func = nullptr;
    };
    static void DestoryString(void* p);
    static void DestoryFieldTable(void* p);
    static void* CopyString(void* p);
    static void* CopyFieldTable(void* p);

    Type type_;
    union {
      bool boolean;
      int8_t int8;
      uint8_t uint8;
      int16_t int16;
      uint16_t uint16;
      int32_t int32;
      uint32_t uint32;
      int64_t int64;
      uint64_t uint64;
      float f;
      double d;
      CustomValue custom;
    };

  public:
#define FieldValueConstructor(cpp_type, type_enum, field)               \
    FieldValue(cpp_type arg) :type_(Type::type_enum), field(arg) { }

    FieldValueConstructor(bool, kBoolean, boolean);
    FieldValueConstructor(int8_t, kShortShortInt, int8);
    FieldValueConstructor(uint8_t, kShortShortUInt, uint8);
    FieldValueConstructor(int16_t, kShortInt, int16);
    FieldValueConstructor(uint16_t, kShortUInt, uint16);
    FieldValueConstructor(int32_t, kLongInt, int32);
    FieldValueConstructor(uint32_t, kLongUInt, uint32);
    FieldValueConstructor(int64_t, kLongLongInt, int64);
    FieldValueConstructor(uint64_t, kLongLongUInt, uint64);
    FieldValueConstructor(float, kFloat, f);
    FieldValueConstructor(double, kDouble, d);
#undef FieldValueConstructor

    FieldValue() :type_(Type::kEmpty) {}
    FieldValue(Type type,
        void* ptr,
        CustomValueDestoryFunc destory_func,
        CustomValueCopyFunc copy_func
        )
      :type_(type) {
        custom.ptr = ptr;
        custom.destory_func = destory_func;
        custom.copy_func = copy_func;
    }

    FieldValue(Type string_type, alpha::Slice arg);
    FieldValue(const FieldArray& arg);
    FieldValue(const FieldTable& arg);
    ~FieldValue();
    FieldValue(const FieldValue& other);
    FieldValue& operator=(const FieldValue& other);

    Type type() const;
    bool is_custom_type_value() const;
    void* Ptr() {
      return &custom;
    }

    const void* CustomPtr() const {
      return custom.ptr;
    }

    uint64_t AsInteger() const;

    template<typename Target>
    Target As() const;

    template<typename Target>
    typename std::add_pointer<Target>::type AsPtr();

    template<typename Target>
    typename std::add_pointer<const Target>::type AsPtr() const;
};

std::ostream& operator<< (std::ostream& os, const FieldValue& v);
}

#endif   /* ----- #ifndef __FIELDVALUE_H__  ----- */
