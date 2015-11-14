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

bool EncoderBase::Encode(CodedWriterBase* w) {
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
  }
  return kDone;
}

void DecoderBase::AddDecodeUnit() {}
}
