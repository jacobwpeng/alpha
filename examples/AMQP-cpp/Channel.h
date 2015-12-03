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

#include <queue>
#include <memory>
#include "MethodArgTypes.h"
#include "Frame.h"

namespace amqp {
class Connection;
class Channel final : public std::enable_shared_from_this<Channel> {
 public:
  explicit Channel(ChannelID channel_id,
                   const std::shared_ptr<Connection>& conn);

  ChannelID id() const { return id_; }
  void AddCachedFrame(FramePtr&& frame);
  FramePtr PopCachedFrame();

 private:
  static const size_t kMaxCachedFrameNum = 100;
  ChannelID id_;
  std::weak_ptr<Connection> conn_;
  std::queue<FramePtr> cached_frames_;
};
}

#endif /* ----- #ifndef __CHANNEL_H__  ----- */
