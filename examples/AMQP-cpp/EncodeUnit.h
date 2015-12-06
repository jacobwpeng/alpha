/*
 * =============================================================================
 *
 *       Filename:  EncodeUnit.h
 *        Created:  11/04/15 15:48:56
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __ENCODEUNIT_H__
#define __ENCODEUNIT_H__

#include <cstdint>
#include <memory>
#include "MethodArgTypes.h"

namespace amqp {

class CodecEnv;
class CodedWriterBase;
class EncodeUnit {
 public:
  virtual ~EncodeUnit() {}
  virtual bool Write(CodedWriterBase* w) = 0;
  virtual size_t ByteSize() const = 0;
};

class BooleanEncodeUnit final : public EncodeUnit {
 public:
  BooleanEncodeUnit(bool val);
  void Add(bool val);
  virtual bool Write(CodedWriterBase* w) override;
  virtual size_t ByteSize() const;

 private:
  static const size_t kMaxByte = 8;
  size_t bits_;
  size_t writed_bits_;
  uint8_t packed_[kMaxByte];
};

class OctetEncodeUnit final : public EncodeUnit {
 public:
  explicit OctetEncodeUnit(int8_t val) : val_(val) {}
  explicit OctetEncodeUnit(uint8_t val) : val_(val) {}
  virtual bool Write(CodedWriterBase* w);
  virtual size_t ByteSize() const override { return 1; };

 private:
  uint8_t val_;
};

class ShortEncodeUnit final : public EncodeUnit {
 public:
  explicit ShortEncodeUnit(int16_t val) : val_(val) {}
  explicit ShortEncodeUnit(uint16_t val) : val_(val) {}
  virtual bool Write(CodedWriterBase* w);
  virtual size_t ByteSize() const override { return 2; };

 private:
  uint16_t val_;
};

class LongEncodeUnit final : public EncodeUnit {
 public:
  explicit LongEncodeUnit(int32_t val) : val_(val) {}
  explicit LongEncodeUnit(uint32_t val) : val_(val) {}
  virtual bool Write(CodedWriterBase* w);
  virtual size_t ByteSize() const override { return 4; };

 private:
  uint32_t val_;
};

class LongLongEncodeUnit final : public EncodeUnit {
 public:
  explicit LongLongEncodeUnit(int64_t val) : val_(val) {}
  explicit LongLongEncodeUnit(uint64_t val) : val_(val) {}
  virtual bool Write(CodedWriterBase* w);
  virtual size_t ByteSize() const override { return 8; };

 private:
  uint64_t val_;
};

class ShortStringEncodeUnit final : public EncodeUnit {
 public:
  explicit ShortStringEncodeUnit(const ShortString& s);
  virtual bool Write(CodedWriterBase* w);
  virtual size_t ByteSize() const override { return 1 + s_.size(); };

 private:
  bool size_done_;
  std::size_t consumed_;
  const ShortString& s_;
};

class LongStringEncodeUnit final : public EncodeUnit {
 public:
  explicit LongStringEncodeUnit(const LongString& s);
  explicit LongStringEncodeUnit(alpha::Slice s);
  virtual bool Write(CodedWriterBase* w);
  virtual size_t ByteSize() const override { return 4 + s_.size(); };

 private:
  LongString saved_;
  bool size_done_;
  std::size_t consumed_;
  const LongString& s_;
};

class FieldValueEncodeUnit final : public EncodeUnit {
 public:
  explicit FieldValueEncodeUnit(const FieldValue& v, const CodecEnv* env);
  virtual bool Write(CodedWriterBase* w);
  virtual size_t ByteSize() const override {
    return underlying_encode_unit_->ByteSize();
  }

 private:
  const CodecEnv* env_;
  std::unique_ptr<EncodeUnit> underlying_encode_unit_;
  const FieldValue& val_;
};

class FieldTableEncodeUnit final : public EncodeUnit {
 public:
  explicit FieldTableEncodeUnit(const FieldTable& ft, const CodecEnv* env);
  virtual bool Write(CodedWriterBase* w);
  virtual size_t ByteSize() const override;

 private:
  const CodecEnv* env_;
  const FieldTable& ft_;
  std::string encoded_;
  std::unique_ptr<LongStringEncodeUnit> long_string_encode_unit_;
};
}

#endif /* ----- #ifndef __ENCODEUNIT_H__  ----- */
