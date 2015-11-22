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
#include <alpha/tcp_connection.h>
#include "Frame.h"
#include "MethodArgTypes.h"

namespace amqp {
class Channel;
class ConnectionMgr;
class Connection final : public std::enable_shared_from_this<Connection> {
 public:
  Connection(ConnectionMgr* owner, alpha::TcpConnectionPtr& conn);
  ~Connection();
  void Close();

  bool HandleFrame(FramePtr&& frame);

  // Channel operation
  void NewChannel();
  void CloseChannel(ChannelID channel_id);
  alpha::TcpConnectionPtr tcp_connection() const;

 private:
  using ChannelMap = std::map<ChannelID, std::unique_ptr<Channel>>;
  void OnChannelOpened(ChannelID channel_id);
  int next_channel_id_;
  ConnectionMgr* owner_;
  alpha::TcpConnectionWeakPtr conn_;
  ChannelMap channels_;
};

using ConnectionPtr = std::shared_ptr<Connection>;
using ConnectionWeakPtr = std::weak_ptr<Connection>;
}

#endif /* ----- #ifndef __CONNECTION_H__  ----- */
