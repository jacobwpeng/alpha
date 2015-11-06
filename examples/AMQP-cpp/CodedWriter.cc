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
#include <alpha/logger.h>

namespace amqp {

TcpConnectionWriter::TcpConnectionWriter(alpha::TcpConnectionPtr& conn)
  :conn_(conn) {
}

bool TcpConnectionWriter::CanWrite(size_t sz) const {
  if (auto conn = conn_.lock()) {
    return conn->WriteBuffer()->SpaceBeforeFull() >= sz;
  }
  return false;
}

size_t TcpConnectionWriter::Write(const void* buf, size_t sz) {
  if (auto conn = conn_.lock()) {
    auto writable_size = std::min(sz, conn->WriteBuffer()->SpaceBeforeFull());
    auto data = reinterpret_cast<const char*>(buf);
    bool ok = conn->Write(alpha::Slice(data, writable_size));
    CHECK(ok) << "Write to TcpConnection failed, writable_size: "
      << writable_size;
    return writable_size;
  }
  return 0;
}

bool MemoryStringWriter::CanWrite(size_t sz) const {
  return true;
}

size_t MemoryStringWriter::Write(const void* buf, size_t sz) {
  s_->append(reinterpret_cast<const char*>(buf), sz);
  return sz;
}

}
