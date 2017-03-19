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
#include <alpha/Logger.h>
#include <alpha/CodedInputStream.h>

NetSvrdFrameCodec::NetSvrdFrameCodec()
    : read_payload_size_(0), frame_(NetSvrdFrame::NullUniquePtr()) {}

NetSvrdFrame::UniquePtr NetSvrdFrameCodec::OnMessage(
    alpha::TcpConnectionPtr conn, alpha::TcpConnectionBuffer* buffer) {
  if (frame_ == nullptr) {
    auto data = buffer->Read();
    if (data.size() < NetSvrdFrame::kHeaderSize) {
      return NetSvrdFrame::NullUniquePtr();
    }
    const NetSvrdFrame* header =
        NetSvrdFrame::CastHeaderOnly(data.data(), data.size());
    if (header == nullptr) {
      LOG_WARNING << "Cast NetSvrdFrame header failed";
      conn->Close();
      return NetSvrdFrame::NullUniquePtr();
    }
    frame_ = NetSvrdFrame::CreateUnique(header->payload_size);
    buffer->ReadAndClear(frame_.get(), NetSvrdFrame::kHeaderSize);
  }

  CHECK(frame_);
  CHECK(read_payload_size_ < frame_->payload_size);

  auto data = buffer->Read();
  if (data.empty()) {
    return NetSvrdFrame::NullUniquePtr();
  }

  auto expect = frame_->payload_size - read_payload_size_;
  auto sz = std::min<size_t>(data.size(), expect);
  memcpy(frame_->payload + read_payload_size_, data.data(), sz);
  buffer->ConsumeBytes(sz);
  read_payload_size_ += sz;
  if (read_payload_size_ == frame_->payload_size) {
    read_payload_size_ = 0;
    return std::move(frame_);
  } else {
    return NetSvrdFrame::NullUniquePtr();
  }
}
