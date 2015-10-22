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

namespace amqp {
  namespace detail {
    template<typename ResultType>
    struct DecoderTypeHelper;

    template<>
    struct DecoderTypeHelper<uint8_t> {
      using type = OctetDecodeUnit;
    };

    template<>
    struct DecoderTypeHelper<uint16_t> {
      using type = ShortDecodeUnit;
    };

    template<>
    struct DecoderTypeHelper<uint32_t> {
      using type = LongDecodeUnit;
    };

    template<>
    struct DecoderTypeHelper<uint64_t> {
      using type = LongLongDecodeUnit;
    };

    template<>
    struct DecoderTypeHelper<FieldTable> {
      using type = FieldTableDecodeUnit;
    };

    template<>
    struct DecoderTypeHelper<ShortString> {
      using type = ShortStringDecodeUnit;
    };

    template<>
    struct DecoderTypeHelper<typename std::conditional<
                              std::is_same<ShortString, LongString>::value,
                              void,
                              LongString
                            >::type >{
      using type = LongStringDecodeUnit;
    };

    template<typename ResultType, typename Enable = void>
    struct DecoderFactory;

    template<typename ResultType>
    struct DecoderFactory<
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
      using DecoderType = typename detail::DecoderTypeHelper<ResultType>::type;
      static DecoderType* New(ResultType* p) {
        return new DecoderType(p);
      }
    };
  }

  DecoderBase::DecoderBase() {
    AddDecodeUnit(&class_id_, &method_id_);
  }

  DecoderBase::~DecoderBase() {
    std::for_each(decode_units_.begin(), decode_units_.end(),
        [](DecodeUnit* unit) { delete unit; });
  }

  int DecoderBase::Decode(alpha::Slice data) {
    Init();
    while (!decode_units_.empty()) {
      auto cur = decode_units_.begin();
      auto rc = (*cur)->ProcessMore(data);
      if (rc != kDone) {
        return rc;
      }
      decode_units_.erase(cur);
      DLOG_INFO << "One decode unit done";
    }
    return kDone;
  }

  void DecoderBase::AddDecodeUnit() { } 

  template<typename Arg, typename... Tail>
  void DecoderBase::AddDecodeUnit(Arg&& arg, Tail&&... tail) {
    decode_units_.push_back(
        detail::DecoderFactory<
          typename std::remove_pointer<
              typename std::remove_reference<
                typename std::remove_cv<Arg>::type
              >::type
          >::type
        >::New(std::forward<Arg>(arg)));
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

DefineArgsDecoder(MethodStartArgs,
    (version_major)(version_minor)(server_properties)(mechanisms)(locales)
);

//DefineArgsDecoder(MethodStartOkArgs,
//    (client_properties)(mechanism)(response)(locale)
//);

#undef DefineArgsDecoder
#undef MemberPtr

}
