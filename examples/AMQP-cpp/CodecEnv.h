/*
 * =============================================================================
 *
 *       Filename:  CodecEnv.h
 *        Created:  11/09/15 11:10:50
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#ifndef  __CODECENV_H__
#define  __CODECENV_H__

#include <alpha/slice.h>
#include "EncodeUnit.h"
#include "DecodeUnit.h"

namespace amqp {
  class FieldValue;
  class CodecEnv {
    public:
      virtual ~CodecEnv() = default;
      virtual std::unique_ptr<FieldValueDecodeUnit> NewDecodeUnit(uint8_t type)
        const = 0;
      virtual uint8_t FieldValueType(const FieldValue& v) const = 0;
      virtual std::unique_ptr<EncodeUnit> NewEncodeUnit(const FieldValue& v)
        const = 0;
  };

  const CodecEnv* GetCodecEnv(alpha::Slice product);
}

#endif   /* ----- #ifndef __CODECENV_H__  ----- */
