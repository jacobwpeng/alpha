/*
 * =============================================================================
 *
 *       Filename:  DecodeUnit.h
 *        Created:  10/21/15 09:48:07
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#ifndef  __DECODEUNIT_H__
#define  __DECODEUNIT_H__

#include <cstdint>
#include <string>
#include <map>
#include <alpha/slice.h>
#include "MethodArgTypes.h"
#include "FieldTable.h"

namespace amqp {

enum DecodeState : int8_t {
  kError = -1,
  kDone = 0,
  kNeedsMore = 1
};

class DecodeUnit {
  public:
    virtual ~DecodeUnit() {}
    /*
     * Process incoming coded data
     * This function may change input argument `data' to consume data
     * Return value:
     *   < 0 means process error
     *   = 0 means process done
     *   > 0 means needs more data to complete
     */
    virtual int ProcessMore(alpha::Slice& data) = 0;
};

class OctetDecodeUnit final : public DecodeUnit {
  public:
    OctetDecodeUnit(uint8_t* res);
    virtual int ProcessMore(alpha::Slice& data);

  private:
    uint8_t* res_;
};

class ShortDecodeUnit final : public DecodeUnit {
  public:
    ShortDecodeUnit(uint16_t* res);
    virtual int ProcessMore(alpha::Slice& data);

  private:
    uint16_t* res_;
};

class LongDecodeUnit final : public DecodeUnit {
  public:
    LongDecodeUnit(uint32_t* res);
    virtual int ProcessMore(alpha::Slice& data);

  private:
    uint32_t* res_;
};

class LongLongDecodeUnit final : public DecodeUnit {
  public:
    LongLongDecodeUnit(uint64_t* res);
    virtual int ProcessMore(alpha::Slice& data);

  private:
    uint64_t* res_;
};

class ShortStringDecodeUnit final : public DecodeUnit {
  public:
    ShortStringDecodeUnit(std::string* res);
    virtual int ProcessMore(alpha::Slice& data);

  private:
    std::string* res_;
    bool read_size_;
    uint8_t sz_;
};

class LongStringDecodeUnit final : public DecodeUnit {
  public:
    LongStringDecodeUnit(std::string* res);
    virtual int ProcessMore(alpha::Slice& data);

  private:
    std::string* res_;
    bool read_size_;
    uint32_t sz_;
};

class TimeStampDecodeUnit final : public DecodeUnit {
  public:
    TimeStampDecodeUnit(Timestamp* res);
    virtual int ProcessMore(alpha::Slice& data);

  private:
    Timestamp* res_;
};

class FieldTableDecodeUnit final : public DecodeUnit {
  public:
    using ResultType = typename std::add_pointer<FieldTable>::type;
    FieldTableDecodeUnit(ResultType res);
    virtual int ProcessMore(alpha::Slice& data);

  private:
    ResultType res_;
    std::string raw_;
    LongStringDecodeUnit underlying_decode_unit_;
};

}

#endif   /* ----- #ifndef __DECODEUNIT_H__  ----- */
