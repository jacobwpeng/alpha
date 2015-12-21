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

#include <cstring>
#include <cstdint>
#include <cstddef>
#include <type_traits>

#ifndef __NETSVRD_FRAME_H__
#define __NETSVRD_FRAME_H__

struct NetSvrdFrame {
  static const uint32_t kMagic = 0;
  static const size_t kHeaderSize = 40;
  static const uint32_t kMaxPayloadSize = 60 * 1024;  // 60KiB
  uint32_t magic;
  uint32_t payload_size;
  uint8_t reserved[32];
  uint8_t payload[];

  const void* data() const { return this; }
  size_t size() const { return payload_size + kHeaderSize; }

  static const NetSvrdFrame* TryCast(const void* data, size_t size) {
    if (size < kHeaderSize) {
      return nullptr;
    }
    const NetSvrdFrame* frame = reinterpret_cast<const NetSvrdFrame*>(data);
    if (frame->magic != kMagic) {
      return nullptr;
    }
    if (size < frame->size()) {
      return nullptr;
    }
    return frame;
  }

  void* operator new(size_t, uint32_t payload_size) {
    void* p = malloc(payload_size + NetSvrdFrame::kHeaderSize);
    memset(p, 0x0, payload_size + NetSvrdFrame::kHeaderSize);
    NetSvrdFrame* f = reinterpret_cast<NetSvrdFrame*>(p);
    f->magic = kMagic;
    f->payload_size = payload_size;
    return p;
  }
  void operator delete(void* p) { free(p); }
};

struct NetSvrdInternalFrame {
  uint32_t magic;
  uint32_t payload_size;
  uint64_t net_server_id;
  uint64_t client_id;
  uint8_t reserved[16];
  uint8_t payload[];
};

static_assert(std::is_pod<NetSvrdFrame>::value,
              "NetSvrdFrame must be POD type");
static_assert(std::is_pod<NetSvrdInternalFrame>::value,
              "NetSvrdFrame must be POD type");
static_assert(offsetof(NetSvrdFrame, payload) == NetSvrdFrame::kHeaderSize,
              "payload must be right after frame header");
static_assert(offsetof(NetSvrdInternalFrame, payload) ==
                  NetSvrdFrame::kHeaderSize,
              "payload must be right after frame header");

#endif /* ----- #ifndef __NETSVRD_FRAME_H__  ----- */
