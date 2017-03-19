/*
 * =============================================================================
 *
 *       Filename:  CodedWriter.cc
 *        Created:  11/05/15 14:50:32
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "CodedWriter.h"
#include <alpha/Logger.h>
#include <alpha/AsyncTcpConnection.h>

namespace amqp {

AsyncTcpConnectionWriter::AsyncTcpConnectionWriter(
    alpha::AsyncTcpConnection* conn)
    : conn_(conn) {}

bool AsyncTcpConnectionWriter::CanWrite(size_t sz) const { return true; }

size_t AsyncTcpConnectionWriter::Write(const void* buf, size_t sz) {
  alpha::Slice data(reinterpret_cast<const char*>(buf), sz);
  conn_->Write(data);
  return sz;
}

bool MemoryStringWriter::CanWrite(size_t sz) const { return true; }

size_t MemoryStringWriter::Write(const void* buf, size_t sz) {
  s_->append(reinterpret_cast<const char*>(buf), sz);
  return sz;
}
}
