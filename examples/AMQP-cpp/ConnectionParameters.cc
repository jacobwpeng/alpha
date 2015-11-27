/*
 * =============================================================================
 *
 *       Filename:  ConnectionParameters.cc
 *        Created:  11/26/15 16:25:49
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "ConnectionParameters.h"

namespace amqp {
ConnectionParameters::ConnectionParameters()
    : port(5672),
      channel_max(0),          // Unlimited channel num (up to 65535)
      heartbeat_delay(600),    // 600s same as default RabbitMQ configure
      connection_attempts(3),  // Try to connect 3 times at most
      socket_timeout(3000),    // 3000ms before timeout
      frame_max(1 << 17),      // 128K same as default RabbitMQ configure
      host("127.0.0.1"),
      vhost("/") {}
}
