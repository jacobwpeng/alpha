/*
 * =============================================================================
 *
 *       Filename:  Channel.cc
 *        Created:  11/22/15 16:12:15
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "Channel.h"

namespace amqp {
Channel::Channel(ChannelID channel_id, ConnectionPtr&& conn)
    : channel_id_(channel_id), conn_(conn) {}

void Channel::Close() {
  if (auto conn = conn_.lock()) {
    conn->CloseChannel(channel_id_);
  }
}
}
