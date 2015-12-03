/*
 * =============================================================================
 *
 *       Filename:  Connection.h
 *        Created:  11/15/15 17:12:36
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include <map>
#include <alpha/AsyncTcpConnection.h>
#include "Frame.h"
#include "FrameCodec.h"
#include "MethodArgTypes.h"

namespace amqp {
class CodecEnv;
class Channel;
class ConnectionMgr;
class Connection final : public std::enable_shared_from_this<Connection> {
 public:
  Connection(const CodecEnv* codec_env, alpha::AsyncTcpConnection* conn);
  //~Connection();
  // void Close();

  // Channel operation
  std::shared_ptr<Channel> NewChannel();
  void CloseChannel(const std::shared_ptr<Channel>& conn);

 private:
  // Handle all frames until frame.channel_id() == stop_channel_id
  // if such frame exists, return that frame
  // else return nullptr
  FramePtr HandleIncomingFrames(ChannelID stop_channel_id,
                                bool cached_only = true);
  void HandleConnectionFrame(FramePtr&& frame);
  std::shared_ptr<Channel> CreateChannel(ChannelID channel_id);
  std::shared_ptr<Channel> FindChannel(ChannelID channel_id) const;
  uint32_t next_channel_id_;
  const CodecEnv* codec_env_;
  alpha::AsyncTcpConnection* conn_;
  FrameWriter w_;
  FrameReader r_;
  std::map<ChannelID, std::shared_ptr<Channel>> channels_;
#if 0
  void CloseChannel(ChannelID channel_id);
  alpha::TcpConnectionPtr tcp_connection() const;

 private:
  using ChannelMap = std::map<ChannelID, std::unique_ptr<Channel>>;
  void OnChannelOpened(ChannelID channel_id);
  int next_channel_id_;
  ConnectionMgr* owner_;
  alpha::TcpConnectionWeakPtr conn_;
  ChannelMap channels_;
#endif
};

using ConnectionPtr = std::shared_ptr<Connection>;
using ConnectionWeakPtr = std::weak_ptr<Connection>;
}

#endif /* ----- #ifndef __CONNECTION_H__  ----- */
