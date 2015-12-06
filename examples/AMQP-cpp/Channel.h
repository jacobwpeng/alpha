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

  void Close();
  bool closed() const { return closed_; }
  ChannelID id() const { return id_; }

  void ExchangeDeclare(alpha::Slice exchange_name, alpha::Slice exchange_type,
                       bool passive, bool durable, bool auto_delete,
                       const FieldTable& arguments = FieldTable());

 private:
  static const size_t kMaxCachedFrameNum = 100;

  void AddCachedFrame(FramePtr&& frame);
  FramePtr PopCachedFrame();
  void set_closed() { closed_ = true; }
  std::shared_ptr<Connection> CheckConnection();

  bool closed_;
  ChannelID id_;
  std::weak_ptr<Connection> conn_;
  std::queue<FramePtr> cached_frames_;
  friend class Connection;
};
}

#endif /* ----- #ifndef __CHANNEL_H__  ----- */
