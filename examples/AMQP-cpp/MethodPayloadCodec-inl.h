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
#include <alpha/compiler.h>
#include "DecodeUnit.h"
#include "EncodeUnit.h"

namespace amqp {
namespace detail {

template <typename ResultType>
struct CodecTypeHelper;

template <>
struct CodecTypeHelper<bool> {
  using EncoderType = BooleanEncodeUnit;
  using DecoderType = BooleanDecodeUnit;
};

template <>
struct CodecTypeHelper<uint8_t> {
  using EncoderType = OctetEncodeUnit;
  using DecoderType = OctetDecodeUnit;
};

template <>
struct CodecTypeHelper<uint16_t> {
  using EncoderType = ShortEncodeUnit;
  using DecoderType = ShortDecodeUnit;
};

template <>
struct CodecTypeHelper<uint32_t> {
  using EncoderType = LongEncodeUnit;
  using DecoderType = LongDecodeUnit;
};

template <>
struct CodecTypeHelper<uint64_t> {
  using EncoderType = LongLongEncodeUnit;
  using DecoderType = LongLongDecodeUnit;
};

template <>
struct CodecTypeHelper<FieldTable> {
  using EncoderType = FieldTableEncodeUnit;
  using DecoderType = FieldTableDecodeUnit;
};

template <>
struct CodecTypeHelper<ShortString> {
  using EncoderType = ShortStringEncodeUnit;
  using DecoderType = ShortStringDecodeUnit;
};

template <>
struct CodecTypeHelper<LongString> {
  using EncoderType = LongStringEncodeUnit;
  using DecoderType = LongStringDecodeUnit;
};

template <typename ResultType, typename Enable = void>
struct CoderFactory;

template <typename ResultType>
struct CoderFactory<ResultType,
                    typename std::enable_if<
                        std::is_same<ResultType, bool>::value ||
                        std::is_same<ResultType, uint8_t>::value ||
                        std::is_same<ResultType, uint16_t>::value ||
                        std::is_same<ResultType, uint32_t>::value ||
                        std::is_same<ResultType, uint64_t>::value ||
                        std::is_same<ResultType, ShortString>::value ||
                        std::is_same<ResultType, LongString>::value>::type> {
  using EncoderType = typename detail::CodecTypeHelper<ResultType>::EncoderType;
  using DecoderType = typename detail::CodecTypeHelper<ResultType>::DecoderType;
  using ResultArgType = typename std::conditional<
      std::is_pod<ResultType>::value, ResultType, const ResultType&>::type;
  static std::unique_ptr<EncoderType> NewEncoder(ResultArgType p,
                                                 const CodecEnv*) {
    return alpha::make_unique<EncoderType>(p);
  }
  static std::unique_ptr<DecoderType> NewDecoder(ResultType* p,
                                                 const CodecEnv*) {
    return alpha::make_unique<DecoderType>(p);
  }
};

template <typename ResultType>
struct CoderFactory<ResultType, typename std::enable_if<std::is_same<
                                    ResultType, FieldTable>::value>::type> {
  using EncoderType = typename detail::CodecTypeHelper<ResultType>::EncoderType;
  using DecoderType = typename detail::CodecTypeHelper<ResultType>::DecoderType;
  using ResultArgType = typename std::conditional<
      std::is_pod<ResultType>::value, ResultType, const ResultType&>::type;
  static std::unique_ptr<EncoderType> NewEncoder(ResultArgType p,
                                                 const CodecEnv* env) {
    return alpha::make_unique<EncoderType>(p, env);
  }
  static std::unique_ptr<DecoderType> NewDecoder(ResultType* p,
                                                 const CodecEnv* env) {
    return alpha::make_unique<DecoderType>(p, env);
  }
};

template <bool BoolEncodeUnit>
struct AddEncodeUnitHelper;

template <>
struct AddEncodeUnitHelper<true> {
  template <typename Arg>
  static void Add(std::vector<std::unique_ptr<EncodeUnit>>* units, Arg&& arg,
                  const CodecEnv* env) {
    CHECK(units);
    BooleanEncodeUnit* bool_encode_unit =
        units->empty()
            ? nullptr
            : dynamic_cast<BooleanEncodeUnit*>(units->rbegin()->get());
    if (bool_encode_unit) {
      bool_encode_unit->Add(arg);
    } else {
      units->push_back(
          CoderFactory<bool>::NewEncoder(std::forward<Arg>(arg), env));
    }
  }
};

template <>
struct AddEncodeUnitHelper<false> {
  template <typename Arg>
  static void Add(std::vector<std::unique_ptr<EncodeUnit>>* units, Arg&& arg,
                  const CodecEnv* env) {
    using RawArgType = typename std::remove_pointer<typename std::remove_cv<
        typename std::remove_reference<Arg>::type>::type>::type;
    CHECK(units);
    units->push_back(
        CoderFactory<RawArgType>::NewEncoder(std::forward<Arg>(arg), env));
  }
};
}

template <typename Arg, typename... Tail>
void EncoderBase::AddEncodeUnit(Arg&& arg, Tail&&... tail) {
  using RawArgType = typename std::remove_pointer<typename std::remove_cv<
      typename std::remove_reference<Arg>::type>::type>::type;

  detail::AddEncodeUnitHelper<std::is_same<RawArgType, bool>::value>::Add(
      &encode_units_, std::forward<Arg>(arg), env_);
  AddEncodeUnit(tail...);
}

template <typename Arg, typename... Tail>
void DecoderBase::AddDecodeUnit(Arg&& arg, Tail&&... tail) {
  decode_units_.push_back(detail::CoderFactory<typename std::remove_pointer<
      typename std::remove_reference<typename std::remove_cv<
          Arg>::type>::type>::type>::NewDecoder(std::forward<Arg>(arg), env_));
  AddDecodeUnit(tail...);
}

template <typename ArgType>
ArgType DecoderBase::GetArg() const {
  // Check ArgType with class_id_ and method_id_
  CHECK(class_id_ == ArgType::kClassID)
      << "Expected class id: " << ArgType::kClassID
      << ", Actual class id: " << class_id_;
  CHECK(method_id_ == ArgType::kMethodID)
      << "Expected method id: " << ArgType::kMethodID
      << ", Actual method id: " << method_id_;
  return *reinterpret_cast<ArgType*>(ptr_);
}

template <typename ArgType>
ArgType GenericMethodArgsDecoder::GetArg() const {
  CHECK(accurate_decoder_) << "No accurate decoder found";
  return accurate_decoder_->GetArg<ArgType>();
}

#define MemberPtr(r, data, i, member) BOOST_PP_COMMA_IF(i) & arg_.member
#define DefineArgsDecoder(ArgType, Seq)                                        \
  class ArgType##Decoder final : public DecoderBase {                          \
   public:                                                                     \
    ArgType##Decoder(const CodecEnv* env) : DecoderBase(env) { ptr_ = &arg_; } \
    virtual void Init() {                                                      \
      AddDecodeUnit(                                                           \
          BOOST_PP_SEQ_FOR_EACH_I(MemberPtr, BOOST_PP_SEQ_SIZE(Seq), Seq));    \
    }                                                                          \
    ArgType Get() const { return arg_; }                                       \
                                                                               \
   private:                                                                    \
    ArgType arg_;                                                              \
  };

#define Member(r, data, i, member) BOOST_PP_COMMA_IF(i) arg_.member
#define DefineArgsEncoder(ArgType, Seq)                                        \
  class ArgType##Encoder final : public EncoderBase {                          \
   public:                                                                     \
    ArgType##Encoder(const ArgType& arg, const CodecEnv* env)                  \
        : EncoderBase(ArgType::kClassID, ArgType::kMethodID, env), arg_(arg) { \
      Init();                                                                  \
    }                                                                          \
    virtual void Init() {                                                      \
      AddEncodeUnit(                                                           \
          BOOST_PP_SEQ_FOR_EACH_I(Member, BOOST_PP_SEQ_SIZE(Seq), Seq));       \
    }                                                                          \
                                                                               \
   private:                                                                    \
    const ArgType& arg_;                                                       \
  };

#define DefineArgsToCodecHelper(ArgType)  \
  template <>                             \
  struct ArgsToCodecHelper<ArgType> {     \
    using EncoderType = ArgType##Encoder; \
    using DecoderType = ArgType##Decoder; \
  };

#define DefineArgsCodec(ArgType, Seq) \
  DefineArgsEncoder(ArgType, Seq);    \
  DefineArgsDecoder(ArgType, Seq);    \
  DefineArgsToCodecHelper(ArgType)

#define DefineRequestToResponseHelper(Req, Resp) \
  template <>                                    \
  struct RequestToResponseHelper<Req> {          \
    using ResponseType = Resp;                   \
  };

#define DefineRequestResponsePair(RequestType, RequestSeq, ResponseType, \
                                  ResponseSeq)                           \
  DefineArgsCodec(RequestType, RequestSeq);                              \
  DefineArgsCodec(ResponseType, ResponseSeq);                            \
  DefineRequestToResponseHelper(RequestType, ResponseType)

DefineRequestResponsePair(
    MethodStartArgs,
    (version_major)(version_minor)(server_properties)(mechanisms)(locales),
    MethodStartOkArgs, (client_properties)(mechanism)(response)(locale));

DefineRequestResponsePair(MethodTuneArgs,
                          (channel_max)(frame_max)(heartbeat_delay),
                          MethodTuneOkArgs,
                          (channel_max)(frame_max)(heartbeat_delay));
DefineRequestResponsePair(MethodOpenArgs, (vhost_path)(capabilities)(insist),
                          MethodOpenOkArgs, BOOST_PP_SEQ_NIL);
DefineRequestResponsePair(MethodCloseArgs,
                          (reply_code)(reply_text)(class_id)(method_id),
                          MethodCloseOkArgs, BOOST_PP_SEQ_NIL);
DefineRequestResponsePair(MethodChannelOpenArgs, (reserved),
                          MethodChannelOpenOkArgs, BOOST_PP_SEQ_NIL);
DefineRequestResponsePair(MethodChannelCloseArgs,
                          (reply_code)(reply_text)(class_id)(method_id),
                          MethodChannelCloseOkArgs, BOOST_PP_SEQ_NIL);
DefineRequestResponsePair(MethodExchangeDeclareArgs,
                          (ticket)(exchange)(type)(passive)(durable)(
                              auto_delete)(internal)(nowait)(arguments),
                          MethodExchangeDeclareOkArgs, BOOST_PP_SEQ_NIL);

#undef DefineArgsToCodecHelper
#undef DefineArgsCodec
#undef DefineArgsEncoder
#undef Member
#undef DefineArgsDecoder
#undef MemberPtr
}
