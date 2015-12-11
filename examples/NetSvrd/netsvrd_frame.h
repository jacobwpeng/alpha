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

#include <cstdint>
#include <cstddef>
#include <type_traits>

#ifndef __NETSVRD_FRAME_H__
#define __NETSVRD_FRAME_H__

struct NetSvrdFrame {
  static const uint64_t kMagic = 0;
  static const size_t kHeaderSize = 40;
  static const uint32_t kMaxPayloadSize = 60 * 1024;  // 60KiB
  uint32_t magic;
  uint32_t payload_size;
  uint8_t reserved[32];
  uint8_t payload[];

  void* operator new(size_t, uint32_t payload_size) {
    return malloc(payload_size + NetSvrdFrame::kHeaderSize);
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
