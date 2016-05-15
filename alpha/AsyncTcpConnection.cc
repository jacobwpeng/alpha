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

#include "AsyncTcpConnection.h"
#include "logger.h"
#include "event_loop.h"
#include "AsyncTcpConnectionException.h"

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

std::string AsyncTcpConnection::Read(size_t bytes, int timeout) {
  if (closed()) {
    throw AsyncTcpConnectionClosed("Read");
  }

  auto buffer = conn_->ReadBuffer();
  std::string result;
  do {
    size_t length;
    auto p = buffer->Read(&length);
    if (p) {
      if (bytes <= result.size() + length) {  // Read from buffer
        CHECK(result.size() < bytes || bytes == 0);
        auto sz = (bytes == 0 ? length : bytes - result.size());
        result.append(p, sz);
        buffer->ConsumeBytes(sz);
        CHECK(result.size() == bytes || (bytes == 0 && !result.empty()))
            << "bytes: " << bytes << ", result.size(): " << result.size()
            << ", sz: " << sz;
        SetIdle();
        return result;
      } else {
        result.append(p, length);
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
      throw AsyncTcpConnectionClosed("Read");
    }
  } while (1);
  SetIdle();
  return result;
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
