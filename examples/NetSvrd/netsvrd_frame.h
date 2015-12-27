/*
 * =============================================================================
 *
 *       Filename:  netsvrd_frame.h
 *        Created:  12/11/15 14:27:57
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once
#ifdef __cplusplus
#if __cplusplus >= 201103L
#define NETSVRD_HAS_FULLY_CXX11_SUPPORT 1
#else
#define NETSVRD_HAS_FULLY_CXX11_SUPPORT 0
#endif
#else
#define NETSVRD_HAS_FULLY_CXX11_SUPPORT 0
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <memory>
#include <cassert>
#include <cstring>
#if NETSVRD_HAS_FULLY_CXX11_SUPPORT
#include <type_traits>
#endif

struct NetSvrdFrame;
struct NetSvrdInternalFrame {
  uint32_t payload_size;
  uint32_t magic;
  uint64_t net_server_id;
  uint64_t client_id;
  uint8_t reserved[16];
  uint8_t payload[];

  inline void CopyPrivateHeadersTo(NetSvrdFrame* frame);
};

struct NetSvrdFrame {
  static const uint32_t kMagic = 0x3e95f3b0u;
  static const size_t kHeaderSize = 40u;
  static const uint32_t kMaxPayloadSize = 60 * 1024u;  // 60KiB
  uint32_t payload_size;
  uint8_t reserved[36];
  uint8_t payload[];

  void* data() { return this; }
  const void* data() const { return this; }
  size_t size() const { return payload_size + kHeaderSize; }
  inline static NetSvrdFrame* Create(size_t payload_size);
  inline static void Destroy(NetSvrdFrame* frame);
  inline static const NetSvrdFrame* CastHeaderOnly(const void* p, size_t size);
#if NETSVRD_HAS_FULLY_CXX11_SUPPORT
  using UniquePtr = std::unique_ptr<NetSvrdFrame, void (*)(NetSvrdFrame*)>;
  inline static UniquePtr CreateUnique(size_t payload_size);
  inline static UniquePtr NullUniquePtr();
#endif
};

#if NETSVRD_HAS_FULLY_CXX11_SUPPORT
static_assert(std::is_pod<NetSvrdFrame>::value,
              "NetSvrdFrame must be POD type");
static_assert(std::is_pod<NetSvrdInternalFrame>::value,
              "NetSvrdInternalFrame must be POD type");
static_assert(offsetof(NetSvrdFrame, payload) == NetSvrdFrame::kHeaderSize,
              "payload must be right after frame header");
static_assert(offsetof(NetSvrdInternalFrame, payload) ==
                  NetSvrdFrame::kHeaderSize,
              "payload must be right after frame header");
static_assert(offsetof(NetSvrdFrame, payload_size) ==
                  offsetof(NetSvrdInternalFrame, payload_size),
              "payload_size must be at the same offset");
#else
typedef int netsvrd_internal_frame_payload_must_be_right_after_frame_header
    [offsetof(NetSvrdInternalFrame, payload) == NetSvrdFrame::kHeaderSize ? 1
                                                                          : -1];
typedef int netsvrd_frame_payload_must_be_right_after_frame_header
    [offsetof(NetSvrdFrame, payload) == NetSvrdFrame::kHeaderSize ? 1 : -1];
typedef int netsvrd_frame_payload_size_must_be_at_same_offset
    [offsetof(NetSvrdFrame, payload_size) ==
             offsetof(NetSvrdInternalFrame, payload_size)
         ? 1
         : -1];
#endif

void NetSvrdInternalFrame::CopyPrivateHeadersTo(NetSvrdFrame* frame) {
  size_t payload_size = frame->payload_size;
  memcpy(frame->data(), this, NetSvrdFrame::kHeaderSize);
  frame->payload_size = payload_size;
}

NetSvrdFrame* NetSvrdFrame::Create(size_t payload_size) {
  char* p = new char[payload_size + NetSvrdFrame::kHeaderSize];
  memset(p, 0x0, payload_size + NetSvrdFrame::kHeaderSize);
  NetSvrdInternalFrame* frame = reinterpret_cast<NetSvrdInternalFrame*>(p);
  frame->payload_size = payload_size;
  frame->magic = kMagic;
  return (NetSvrdFrame*)frame;
}

void NetSvrdFrame::Destroy(NetSvrdFrame* frame) {
  if (frame == NULL) return;
  NetSvrdInternalFrame* internal_frame =
      reinterpret_cast<NetSvrdInternalFrame*>(frame);
  assert(internal_frame->magic == kMagic);
  char* p = reinterpret_cast<char*>(frame);
  delete[] p;
}

const NetSvrdFrame* NetSvrdFrame::CastHeaderOnly(const void* p, size_t size) {
  if (size < kHeaderSize) {
    return NULL;
  }
  const NetSvrdInternalFrame* frame =
      reinterpret_cast<const NetSvrdInternalFrame*>(p);
  if (frame->magic == kMagic && frame->payload_size <= kMaxPayloadSize) {
    return (NetSvrdFrame*)frame;
  } else {
    return NULL;
  }
}

#if NETSVRD_HAS_FULLY_CXX11_SUPPORT
using UniquePtr = std::unique_ptr<NetSvrdFrame, void (*)(NetSvrdFrame*)>;
NetSvrdFrame::UniquePtr NetSvrdFrame::CreateUnique(size_t payload_size) {
  return UniquePtr(Create(payload_size), NetSvrdFrame::Destroy);
}
UniquePtr NetSvrdFrame::NullUniquePtr() {
  return UniquePtr(nullptr, NetSvrdFrame::Destroy);
}
#endif
