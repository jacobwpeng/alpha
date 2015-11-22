/*
 * =============================================================================
 *
 *       Filename:  Channel.h
 *        Created:  11/22/15 16:01:21
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include "Connection.h"

namespace amqp {
class Channel final {
 public:
  explicit Channel(ChannelID channel_id, ConnectionPtr&& conn);
  void Close();

 private:
  ChannelID channel_id_;
  ConnectionWeakPtr conn_;
};
}

#endif /* ----- #ifndef __CHANNEL_H__  ----- */
