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

#pragma once

#include <memory>
#include <vector>
#include <type_traits>
#include "MethodArgs.h"
#include "Frame.h"

namespace amqp {

class Frame;
class CodedWriterBase;
class EncodeUnit;
class DecodeUnit;
class CodecEnv;

class EncoderBase {
 public:
  virtual ~EncoderBase() = default;
  void WriteTo(CodedWriterBase* w);
  size_t ByteSize() const;

 protected:
  EncoderBase(ClassID class_id, MethodID method_id, const CodecEnv* env);
  void AddEncodeUnit();
  template <typename Arg, typename... Tail>
  void AddEncodeUnit(Arg&& arg, Tail&&... tail);
  virtual void Init() = 0;

 private:
  const CodecEnv* env_;
  std::vector<std::unique_ptr<EncodeUnit>> encode_units_;
};

class DecoderBase {
 public:
  virtual ~DecoderBase() = default;
  void Decode(FramePtr&& frame);
  ClassID class_id() const { return class_id_; }
  MethodID method_id() const { return method_id_; }

  template <typename ArgType>
  ArgType GetArg() const;

 protected:
  DecoderBase(const CodecEnv* env);
  virtual void Init() = 0;
  void AddDecodeUnit();
  template <typename Arg, typename... Tail>
  void AddDecodeUnit(Arg&& arg, Tail&&... tail);

  void* ptr_;

 private:
  bool inited_;
  ClassID class_id_;
  MethodID method_id_;
  const CodecEnv* env_;
  std::vector<std::unique_ptr<DecodeUnit>> decode_units_;
};

class GenericMethodArgsDecoder final {
 public:
  GenericMethodArgsDecoder(const CodecEnv* codec_env);
  void Decode(FramePtr&& frame);
  void Reset();
  const DecoderBase* accurate_decoder() const;

  template <typename ArgType>
  ArgType GetArg() const;

 private:
  std::unique_ptr<DecoderBase> CreateAccurateDecoder(ClassID class_id,
                                                     MethodID method_id);
  static const size_t kMinSizeToRecognize = sizeof(ClassID) + sizeof(MethodID);
  const CodecEnv* codec_env_;
  std::unique_ptr<DecoderBase> accurate_decoder_;
};

template <typename Args>
struct ArgsToCodecHelper;

template <typename RequestType>
struct RequestToResponseHelper;
}

#include "MethodPayloadCodec-inl.h"

