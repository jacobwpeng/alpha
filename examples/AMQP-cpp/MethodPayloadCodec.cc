/*
 * =============================================================================
 *
 *       Filename:  MethodPayloadCodec.cc
 *        Created:  11/12/15 11:17:55
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "MethodPayloadCodec.h"

namespace amqp {

EncoderBase::EncoderBase(ClassID class_id, MethodID method_id,
                         const CodecEnv* env)
    : env_(env) {
  AddEncodeUnit(class_id);
  AddEncodeUnit(method_id);
}

void EncoderBase::WriteTo(CodedWriterBase* w) {
  for (auto& unit : encode_units_) {
    unit->Write(w);
  }
}

size_t EncoderBase::ByteSize() const {
  size_t total = 0;
  for (const auto& unit : encode_units_) {
    total += unit->ByteSize();
  }
  return total;
}

void EncoderBase::AddEncodeUnit() {}

DecoderBase::DecoderBase(const CodecEnv* env)
    : inited_(false), class_id_(0), method_id_(0), env_(env) {
  AddDecodeUnit(&class_id_, &method_id_);
}

void DecoderBase::Decode(FramePtr&& frame) {
  if (!inited_) {
    Init();
    inited_ = true;
  }

  alpha::Slice data(frame->payload());
  while (!decode_units_.empty()) {
    auto cur = decode_units_.begin();
    auto rc = (*cur)->ProcessMore(data);
    // Assume all method args arrived in one frame
    CHECK(rc == kDone) << "Method frame should not be seperated";
    decode_units_.erase(cur);
  }
}

void DecoderBase::AddDecodeUnit() {}

GenericMethodArgsDecoder::GenericMethodArgsDecoder(const CodecEnv* codec_env)
    : codec_env_(codec_env) {}

void GenericMethodArgsDecoder::Reset() { accurate_decoder_.reset(); }

const DecoderBase* GenericMethodArgsDecoder::accurate_decoder() const {
  return accurate_decoder_.get();
}

void GenericMethodArgsDecoder::Decode(FramePtr&& frame) {
  CHECK(frame->type() == Frame::Type::kMethod)
      << "Invalid frame type: " << frame->type();
  alpha::Slice payload(frame->payload());
  // TODO: throw a ConnectionException
  CHECK(payload.size() >= kMinSizeToRecognize);
  if (accurate_decoder_ == nullptr) {
    ClassID class_id;
    MethodID method_id;
    ShortDecodeUnit class_id_decode_unit(&class_id);
    ShortDecodeUnit method_id_decode_unit(&method_id);

    auto ret = class_id_decode_unit.ProcessMore(payload);
    CHECK(ret == kDone);
    ret = method_id_decode_unit.ProcessMore(payload);
    CHECK(ret == kDone);

    accurate_decoder_ = CreateAccurateDecoder(class_id, method_id);
    payload = frame->payload();
  }
  accurate_decoder_->Decode(std::move(frame));
}

std::unique_ptr<DecoderBase> GenericMethodArgsDecoder::CreateAccurateDecoder(
    ClassID class_id, MethodID method_id) {
#define Case(Type)      \
  case Type::kMethodID: \
    return alpha::make_unique<Type##Decoder>(codec_env_)

  if (class_id == kClassConnectionID) {
    switch (method_id) {
      Case(MethodStartArgs);
      Case(MethodTuneArgs);
      Case(MethodOpenOkArgs);
      Case(MethodCloseArgs);
      Case(MethodCloseOkArgs);
    }
  } else if (class_id == kClassChannelID) {
    switch (method_id) {
      Case(MethodChannelOpenArgs);
      Case(MethodChannelOpenOkArgs);
      Case(MethodChannelCloseArgs);
      Case(MethodChannelCloseOkArgs);
    }
  }
  CHECK(false) << "Invalid class_id - method_id combination, "
               << " class_id: " << class_id << ", method_id: " << method_id;
  return nullptr;

#undef Case
}
}
