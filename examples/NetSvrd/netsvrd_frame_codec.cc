/*
 * =============================================================================
 *
 *       Filename:  netsvrd_frame_codec.cc
 *        Created:  12/10/15 15:21:22
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "netsvrd_frame_codec.h"
#include <alpha/Endian.h>
#include <alpha/logger.h>
#include <alpha/CodedInputStream.h>

NetSvrdFrameCodec::NetSvrdFrameCodec() : read_payload_size_(0) {}

std::unique_ptr<NetSvrdFrame> NetSvrdFrameCodec::OnMessage(
    alpha::TcpConnectionPtr conn, alpha::TcpConnectionBuffer* buffer) {
  if (frame_ == nullptr) {
    auto data = buffer->Read();
    if (data.size() < NetSvrdFrame::kHeaderSize) {
      return nullptr;
    }
    alpha::CodedInputStream stream(data);
    uint32_t magic;
    bool ok = stream.ReadLittleEndianUInt32(&magic);
    CHECK(ok);
    if (magic != NetSvrdFrame::kMagic) {
      LOG_WARNING << "Check magic failed, expected: " << NetSvrdFrame::kMagic
                  << ", actual: " << frame_->magic;
      conn->Close();
      return nullptr;
    }
    uint32_t payload_size;
    ok = stream.ReadLittleEndianUInt32(&payload_size);
    CHECK(ok);
    if (payload_size > NetSvrdFrame::kMaxPayloadSize) {
      LOG_WARNING << "Payload is too large, max: "
                  << NetSvrdFrame::kMaxPayloadSize
                  << ", actual: " << frame_->payload_size;
      conn->Close();
      return nullptr;
    }
    frame_ = std::unique_ptr<NetSvrdFrame>(new (payload_size) NetSvrdFrame);
    buffer->ReadAndClear(reinterpret_cast<char*>(frame_.get()),
                         NetSvrdFrame::kHeaderSize);
  }

  CHECK(frame_);
  CHECK(read_payload_size_ < frame_->payload_size);

  auto data = buffer->Read();
  if (data.empty()) {
    return nullptr;
  }

  auto sz = std::min<size_t>(data.size(), frame_->payload_size);
  memcpy(frame_->payload + read_payload_size_, data.data(), sz);
  read_payload_size_ += sz;
  if (read_payload_size_ == frame_->payload_size) {
    read_payload_size_ = 0;
    return std::move(frame_);
  } else {
    return nullptr;
  }
}
