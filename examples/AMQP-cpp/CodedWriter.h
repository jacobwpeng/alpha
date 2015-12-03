/*
 * =============================================================================
 *
 *       Filename:  CodedWriter.h
 *        Created:  11/05/15 14:48:56
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __CODEDWRITER_H__
#define __CODEDWRITER_H__

#include <cstdint>
#include <string>
#include <memory>
#include <alpha/AsyncTcpConnection.h>

namespace amqp {

using std::size_t;
class CodedWriterBase {
 public:
  virtual ~CodedWriterBase() {}
  virtual bool CanWrite(size_t bytes) const = 0;
  virtual size_t Write(const void*, size_t sz) = 0;
};

class AsyncTcpConnectionWriter final : public CodedWriterBase {
 public:
  explicit AsyncTcpConnectionWriter(alpha::AsyncTcpConnection* conn);

  virtual bool CanWrite(size_t sz) const override;
  virtual size_t Write(const void* buf, size_t sz) override;

 private:
  alpha::AsyncTcpConnection* conn_;
};

class MemoryStringWriter final : public CodedWriterBase {
 public:
  explicit MemoryStringWriter(std::string* s) : s_(s) {}

  virtual bool CanWrite(size_t sz) const override;
  virtual size_t Write(const void* buf, size_t sz) override;

 private:
  std::string* s_;
};
}

#endif /* ----- #ifndef __CODEDWRITER_H__  ----- */
