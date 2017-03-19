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

#pragma once

#include <memory>
#include <functional>
#include <alpha/TcpConnection.h>
#include "netsvrd_frame.h"

class NetSvrdFrameCodec final {
 public:
  NetSvrdFrameCodec();
  NetSvrdFrame::UniquePtr OnMessage(alpha::TcpConnectionPtr conn,
                                    alpha::TcpConnectionBuffer* buffer);

 private:
  uint32_t read_payload_size_;
  NetSvrdFrame::UniquePtr frame_;
};

