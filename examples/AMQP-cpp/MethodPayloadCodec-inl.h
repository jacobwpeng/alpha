/*
 * =============================================================================
 *
 *       Filename:  MethodPayloadCodec-inl.h
 *        Created:  10/22/15 11:07:43
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#ifndef __METHODPAYLOADCODEC_H__
#error This file may only be included from MethodPayloadCodec.h.
#endif

#include <algorithm>
#include <boost/preprocessor.hpp>
#include <alpha/logger.h>
#include "DecodeUnit.h"
#include "EncodeUnit.h"

namespace amqp {
  namespace detail {
    template<typename ResultType>
    struct CodecTypeHelper;

    template<>
    struct CodecTypeHelper<uint8_t> {
      using EncoderType = OctetEncodeUnit;
      using DecoderType = OctetDecodeUnit;
    };

    template<>
    struct CodecTypeHelper<uint16_t> {
      using EncoderType = ShortEncodeUnit;
      using DecoderType = ShortDecodeUnit;
    };

    template<>
    struct CodecTypeHelper<uint32_t> {
      using EncoderType = LongEncodeUnit;
      using DecoderType = LongDecodeUnit;
    };

    template<>
    struct CodecTypeHelper<uint64_t> {
      using EncoderType = LongLongEncodeUnit;
      using DecoderType = LongLongDecodeUnit;
    };

    template<>
    struct CodecTypeHelper<FieldTable> {
      using EncoderType = FieldTableEncodeUnit;
      using DecoderType = FieldTableDecodeUnit;
    };

    template<>
    struct CodecTypeHelper<ShortString> {
      using EncoderType = ShortStringEncodeUnit;
      using DecoderType = ShortStringDecodeUnit;
    };

    template<>
    struct CodecTypeHelper<LongString> {
      using EncoderType = LongStringEncodeUnit;
      using DecoderType = LongStringDecodeUnit;
    };

    template<typename ResultType, typename Enable = void>
    struct CoderFactory;

    template<typename ResultType>
    struct CoderFactory<
      ResultType,
      typename std::enable_if<
        std::is_same<ResultType, uint8_t>::value
        || std::is_same<ResultType, uint16_t>::value
        || std::is_same<ResultType, uint32_t>::value
        || std::is_same<ResultType, uint64_t>::value
        || std::is_same<ResultType, FieldTable>::value
        || std::is_same<ResultType, ShortString>::value
        || std::is_same<ResultType, LongString>::value
      >::type
    > {
      using EncoderType = typename
        detail::CodecTypeHelper<ResultType>::EncoderType;
      using DecoderType = typename 
        detail::CodecTypeHelper<ResultType>::DecoderType;
      using ResultArgType = typename std::conditional<
                                      std::is_pod<ResultType>::value,
                                      ResultType,
                                      const ResultType&
                                    >::type;
      static std::unique_ptr<EncoderType> NewEncoder(ResultArgType p) {
        return std::unique_ptr<EncoderType>(new EncoderType(p));
      }
      static std::unique_ptr<DecoderType> NewDecoder(ResultType* p) {
        return std::unique_ptr<DecoderType>(new DecoderType(p));
      }
    };
  }

  EncoderBase::EncoderBase(ClassID class_id, MethodID method_id)
    :inited_(false) {
    AddEncodeUnit(class_id);
    AddEncodeUnit(method_id);
  }

  bool EncoderBase::Encode(CodedWriterBase* w) {
    if (!inited_) {
      Init();
      inited_ = true;
    }

    while (!encode_units_.empty()) {
      auto cur = encode_units_.begin();
      bool done = (*cur)->Write(w);
      if (!done) {
        break;
      }
      encode_units_.erase(cur);
    }
    return encode_units_.empty();
  }

  void EncoderBase::AddEncodeUnit() {}

  template<typename Arg, typename... Tail>
  void EncoderBase::AddEncodeUnit(Arg&& arg, Tail&&... tail) {
    encode_units_.push_back(
        detail::CoderFactory<
          typename std::remove_pointer<
              typename std::remove_cv<
                typename std::remove_reference<Arg>::type
              >::type
          >::type
        >::NewEncoder(std::forward<Arg>(arg)));
    AddEncodeUnit(tail...);
  }

  DecoderBase::DecoderBase()
    :inited_(false) {
    AddDecodeUnit(&class_id_, &method_id_);
  }

  int DecoderBase::Decode(alpha::Slice data) {
    if (!inited_) { 
      Init(); 
      inited_ = true;
    }

    while (!decode_units_.empty()) {
      auto cur = decode_units_.begin();
      auto rc = (*cur)->ProcessMore(data);
      if (rc != kDone) {
        return rc;
      }
      decode_units_.erase(cur);
      //DLOG_INFO << decode_units_.size() << " DecodeUnit(s) left"
      //  << ", data.size() = " << data.size();
    }
    return kDone;
  }

  void DecoderBase::AddDecodeUnit() { } 

  template<typename Arg, typename... Tail>
  void DecoderBase::AddDecodeUnit(Arg&& arg, Tail&&... tail) {
    decode_units_.push_back(
        detail::CoderFactory<
          typename std::remove_pointer<
              typename std::remove_reference<
                typename std::remove_cv<Arg>::type
              >::type
          >::type
        >::NewDecoder(std::forward<Arg>(arg)));
    AddDecodeUnit(tail...);
  }

  template<typename ArgType>
  ArgType DecoderBase::Get() const {
    // Check ArgType with class_id_ and method_id_
    CHECK(class_id_ == ArgType::kClassID)
      << "Expected class id: " << ArgType::kClassID
      << ", Real class id: " << class_id_;
    CHECK(method_id_ == ArgType::kMethodID)
      << "Expected method id: " << ArgType::kMethodID
      << ", Real method id: " << method_id_;
    return *reinterpret_cast<ArgType*>(ptr_);
  }

#define Member(r, data, i, member) BOOST_PP_COMMA_IF(i) arg_.member
#define MemberPtr(r, data, i, member) BOOST_PP_COMMA_IF(i) &arg_.member
#define DefineArgsDecoder(ArgType, Seq)                              \
class ArgType##Decoder final : public DecoderBase {                  \
  public:                                                            \
    ArgType##Decoder() {                                             \
        ptr_ = &arg_;                                                \
    }                                                                \
    virtual void Init() {                                            \
      AddDecodeUnit(BOOST_PP_SEQ_FOR_EACH_I(                         \
            MemberPtr, BOOST_PP_SEQ_SIZE(Seq), Seq));                \
    }                                                                \
    ArgType Get() {                                                  \
      return arg_;                                                   \
    }                                                                \
  private:                                                           \
    ArgType arg_;                                                    \
};

#define DefineArgsEncoder(ArgType, CID, MID, Seq)                    \
class ArgType##Encoder final : public EncoderBase {                  \
  public:                                                            \
    ArgType##Encoder(const ArgType& arg)                             \
      : EncoderBase(CID, MID), arg_(arg) {                           \
    }                                                                \
    virtual void Init() {                                            \
      AddEncodeUnit(BOOST_PP_SEQ_FOR_EACH_I(                         \
            Member, BOOST_PP_SEQ_SIZE(Seq), Seq));                   \
    }                                                                \
  private:                                                           \
    const ArgType& arg_;                                             \
};

DefineArgsDecoder(MethodStartArgs,
    (version_major)(version_minor)(server_properties)(mechanisms)(locales)
);

DefineArgsEncoder(MethodStartOkArgs, 10, 11,
    (client_properties)(mechanism)(response)(locale)
);

//DefineArgsDecoder(MethodStartOkArgs,
//    (client_properties)(mechanism)(response)(locale)
//);

#undef DefineArgsDecoder
#undef MemberPtr

}
