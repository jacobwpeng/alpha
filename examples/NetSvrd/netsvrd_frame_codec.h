/*
 * =============================================================================
 *
 *       Filename:  netsvrd_frame_codec.h
 *        Created:  12/10/15 15:19:46
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __NETSVRD_FRAME_CODEC_H__
#define __NETSVRD_FRAME_CODEC_H__

#include <memory>
#include <functional>
#include <alpha/tcp_connection.h>
#include "netsvrd_frame.h"

class NetSvrdFrameCodec final {
 public:
  NetSvrdFrameCodec();
  std::unique_ptr<NetSvrdFrame> OnMessage(alpha::TcpConnectionPtr conn,
                                          alpha::TcpConnectionBuffer* buffer);

 private:
  uint32_t read_payload_size_;
  std::unique_ptr<NetSvrdFrame> frame_;
};

#endif /* ----- #ifndef __NETSVRD_FRAME_CODEC_H__  ----- */
