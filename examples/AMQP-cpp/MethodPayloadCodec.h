/*
 * =============================================================================
 *
 *       Filename:  MethodPayloadCodec.h
 *        Created:  10/21/15 16:40:02
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#ifndef  __METHODPAYLOADCODEC_H__
#define  __METHODPAYLOADCODEC_H__

#include <memory>
#include <type_traits>
#include "MethodArgs.h"

namespace amqp {

class Frame;
class CodedWriterBase;
class EncodeUnit;
class DecodeUnit;
class EncoderBase {
  public:
    virtual ~EncoderBase() = default;
    virtual void Init() = 0;

    void Encode(CodedWriterBase* w);

    void AddEncodeUnit();
    template<typename Arg, typename... Tail>
    void AddEncodeUnit(Arg&& arg, Tail&&... tail);
};

class DecoderBase {
  public:
    virtual ~DecoderBase() = default;
    virtual void Init() = 0;
    int Decode(alpha::Slice data);

    void AddDecodeUnit();
    template<typename Arg, typename... Tail> 
    void AddDecodeUnit(Arg&& arg, Tail&&... tail);
    template<typename ArgType>
    ArgType Get() const;

  protected:
    DecoderBase();
    void* ptr_;

  private:
    std::vector<std::unique_ptr<DecodeUnit>> decode_units_;
    ClassID class_id_;
    MethodID method_id_;
    bool inited_;
};

}

#include "MethodPayloadCodec-inl.h"

#endif   /* ----- #ifndef __METHODPAYLOADCODEC_H__  ----- */
