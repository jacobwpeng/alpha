/*
 * =============================================================================
 *
 *       Filename:  ConnectionParameters.h
 *        Created:  11/26/15 16:24:59
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include <cstdint>
#include <string>

namespace amqp {
struct ConnectionParameters {
  ConnectionParameters();

  int port;
  uint16_t channel_max;
  uint16_t heartbeat_delay;
  uint16_t connection_attempts;
  uint16_t socket_timeout;
  uint32_t frame_max;
  std::string host;
  std::string vhost;
};
}

