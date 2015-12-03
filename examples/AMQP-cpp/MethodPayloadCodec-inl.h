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
  using EncoderType = OctetEncodeUnit;
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
}

template <typename Arg, typename... Tail>
void EncoderBase::AddEncodeUnit(Arg&& arg, Tail&&... tail) {
  encode_units_.push_back(
      detail::CoderFactory<typename std::remove_pointer<typename std::remove_cv<
          typename std::remove_reference<Arg>::type>::type>::type>::
          NewEncoder(std::forward<Arg>(arg), env_));
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

#define DefineArgsCodec(ArgType, Seq) \
  DefineArgsEncoder(ArgType, Seq);    \
  DefineArgsDecoder(ArgType, Seq)

DefineArgsCodec(MethodStartArgs, (version_major)(version_minor)(
                                     server_properties)(mechanisms)(locales));
DefineArgsCodec(MethodStartOkArgs,
                (client_properties)(mechanism)(response)(locale));
DefineArgsCodec(MethodTuneArgs, (channel_max)(frame_max)(heartbeat_delay));
DefineArgsCodec(MethodTuneOkArgs, (channel_max)(frame_max)(heartbeat_delay));
DefineArgsCodec(MethodOpenArgs, (vhost_path)(capabilities)(insist));
DefineArgsCodec(MethodOpenOkArgs, BOOST_PP_SEQ_NIL);
DefineArgsCodec(MethodCloseArgs, (reply_code)(reply_text)(class_id)(method_id));
DefineArgsCodec(MethodCloseOkArgs, BOOST_PP_SEQ_NIL);
DefineArgsCodec(MethodChannelOpenArgs, (reserved));
DefineArgsCodec(MethodChannelOpenOkArgs, BOOST_PP_SEQ_NIL);
DefineArgsCodec(MethodChannelCloseArgs,
                (reply_code)(reply_text)(class_id)(method_id));
DefineArgsCodec(MethodChannelCloseOkArgs, BOOST_PP_SEQ_NIL);

#undef DefineArgsCodec
#undef DefineArgsEncoder
#undef Member
#undef DefineArgsDecoder
#undef MemberPtr
}
