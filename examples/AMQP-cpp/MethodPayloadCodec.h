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

#ifndef __METHODPAYLOADCODEC_H__
#define __METHODPAYLOADCODEC_H__

#include <memory>
#include <vector>
#include <type_traits>
#include "MethodArgs.h"

namespace amqp {

class Frame;
class CodedWriterBase;
class EncodeUnit;
class DecodeUnit;
class CodecEnv;
class EncoderBase {
 public:
  virtual ~EncoderBase() = default;
  bool Encode(CodedWriterBase* w);
  size_t ByteSize() const;

 protected:
  EncoderBase(ClassID class_id, MethodID method_id, const CodecEnv* env);
  void AddEncodeUnit();
  template <typename Arg, typename... Tail>
  void AddEncodeUnit(Arg&& arg, Tail&&... tail);
  virtual void Init() = 0;

 private:
  bool inited_;
  const CodecEnv* env_;
  std::vector<std::unique_ptr<EncodeUnit>> encode_units_;
};

class DecoderBase {
 public:
  virtual ~DecoderBase() = default;
  int Decode(alpha::Slice data);

  template <typename ArgType>
  ArgType Get() const;

 protected:
  DecoderBase(const CodecEnv* env);
  virtual void Init() = 0;
  void AddDecodeUnit();
  template <typename Arg, typename... Tail>
  void AddDecodeUnit(Arg&& arg, Tail&&... tail);

  void* ptr_;

 private:
  const CodecEnv* env_;
  std::vector<std::unique_ptr<DecodeUnit>> decode_units_;
  ClassID class_id_;
  MethodID method_id_;
  bool inited_;
};
}

#include "MethodPayloadCodec-inl.h"

#endif /* ----- #ifndef __METHODPAYLOADCODEC_H__  ----- */
