/*
 * =============================================================================
 *
 *       Filename:  AsyncTcpConnection.cc
 *        Created:  12/01/15 10:48:32
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <alpha/AsyncTcpConnection.h>
#include <alpha/Logger.h>
#include <alpha/EventLoop.h>
#include <alpha/AsyncTcpConnectionException.h>

namespace alpha {
AsyncTcpConnection::AsyncTcpConnection(TcpConnectionPtr& conn,
                                       AsyncTcpConnectionCoroutine* co)
    : status_(Status::kIdle), conn_(conn), co_(co) {}

void AsyncTcpConnection::Write(const void* data, size_t size) {
  Write(alpha::Slice(reinterpret_cast<const char*>(data), size));
}

void AsyncTcpConnection::Write(alpha::Slice data) {
  if (closed()) {
    throw AsyncTcpConnectionClosed("Write");
  }

  do {
    auto max = conn_->BytesCanWrite();
    auto sz = std::min(max, data.size());
    conn_->Write(data.subslice(0, sz));
    data.Advance(sz);
    if (data.empty()) {
      break;
    }
    SetWaitingWriteDone();
    co_->Yield();
    if (closed()) {
      throw AsyncTcpConnectionClosed("Write");
    }
  } while (1);
  SetIdle();
}

void AsyncTcpConnection::WaitWriteDone() {
  size_t length;
  auto p = conn_->WriteBuffer()->Read(&length);
  if (!p) return;

  DLOG_INFO << length << " bytes to send";
  SetWaitingWriteDone();
  co_->Yield();
}

size_t AsyncTcpConnection::ReadFull(IOBuffer* io_buffer,
                                    size_t buffer_size,
                                    int timeout) {
  if (closed()) {
    throw AsyncTcpConnectionClosed("Read");
  }
  if (buffer_size == 0) {
    return 0;
  }

  size_t nread = 0;
  auto buffer = conn_->ReadBuffer();
  do {
    size_t length;
    auto p = buffer->Read(&length);
    if (p) {
      if (buffer_size <= nread + length) {  // Read from buffer
        CHECK(nread <= buffer_size);
        auto sz = buffer_size - nread;
        memcpy(io_buffer->data() + nread, p, sz);
        buffer->ConsumeBytes(sz);
        nread += sz;
        SetIdle();
        return nread;
      } else {
        memcpy(io_buffer->data() + nread, p, length);
        nread += length;
        buffer->ConsumeBytes(length);
      }
    }
    SetWaitingMessage();
    if (timeout == kNoTimeout) {
      co_->Yield();
    } else {
      co_->YieldWithTimeout(timeout);
    }
    if (co_->timeout()) {
      throw alpha::AsyncTcpConnectionOperationTimeout();
    }
    if (closed()) {
      return nread;
    }
  } while (1);
  SetIdle();
  return nread;
}

std::string AsyncTcpConnection::ReadCached(size_t bytes) {
  auto buffer = conn_->ReadBuffer();
  size_t length;
  auto p = buffer->Read(&length);
  if (!p) return "";
  if (bytes == 0) {  // Read all
    std::string result(p, length);
    buffer->ConsumeBytes(length);
    return result;
  }
  auto sz = std::min(bytes, length);
  std::string result = std::string(p, sz);
  buffer->ConsumeBytes(sz);
  return result;
}

char* AsyncTcpConnection::PeekCached(size_t* length) const {
  return conn_->ReadBuffer()->Read(length);
}

void AsyncTcpConnection::Close() {
  if (!closed()) {
    conn_->Close();
    status_ = Status::kWaitingClose;
    co_->Yield();
  }
}

bool AsyncTcpConnection::HasCachedData() const { return CachedDataSize() != 0; }

size_t AsyncTcpConnection::CachedDataSize() const {
  size_t length;
  conn_->ReadBuffer()->Read(&length);
  return length;
}
}
