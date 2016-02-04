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
  auto data = conn_->WriteBuffer()->Read();
  if (data.empty()) {
    return;
  }
  DLOG_INFO << data.size() << " bytes to send";
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
    auto data = buffer->Read();
    if (!data.empty()) {
      if (bytes <= result.size() + data.size()) {  // Read from buffer
        CHECK(result.size() < bytes || bytes == 0);
        auto sz = (bytes == 0 ? data.size() : bytes - result.size());
        result += data.subslice(0, sz).ToString();
        buffer->ConsumeBytes(sz);
        CHECK(result.size() == bytes || (bytes == 0 && !result.empty()))
            << "bytes: " << bytes << ", result.size(): " << result.size()
            << ", sz: " << sz;
        SetIdle();
        return result;
      } else {
        result.append(data.data(), data.size());
        buffer->ConsumeBytes(data.size());
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
  auto data = buffer->Read();
  if (bytes == 0) {
    buffer->ConsumeBytes(data.size());
    return data.ToString();
  }
  auto sz = std::min(bytes, data.size());
  std::string result = data.subslice(0, sz).ToString();
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
  return conn_->ReadBuffer()->Read().size();
}
}
