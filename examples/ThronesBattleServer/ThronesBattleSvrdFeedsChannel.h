/*
 * =============================================================================
 *
 *       Filename:  ThronesBattleSvrdFeedsChannel.h
 *        Created:  01/05/16 15:56:21
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  用来发送Feeds到Netsvrd并有feedssvrd处理
 *                  feeds和战斗不同，不需要像TaskBroker一样保证非常高的可靠性
 *                  但是由于同时可能会写大量feeds, UDP方式可能会导致大量丢包
 *                  所以只是简单通过TCP方式写到Netsvrd
 * =============================================================================
 */

#pragma once

#include <string>
#include <sstream>
#include <alpha/UDPSocket.h>
#include <alpha/AsyncTcpClient.h>
#include <alpha/AsyncTcpConnection.h>
#include <alpha/AsyncTcpConnectionCoroutine.h>
#include <google/protobuf/message.h>
#include "ThronesBattleSvrdDef.h"
#include "ext/netsvrd_frame.h"

namespace ThronesBattle {
static const unsigned kThronesBattleLose = 450;
static const unsigned kThronesBattleWin = 451;
static const unsigned kThronesBattleBye = 452;
static const unsigned kThronesBattleEventType = 22;
static const unsigned kThronesBattleCampRank[kCampIDMax] = {
    453, 454, 455, 456, 456, 456, 456, 456};
class FeedsChannel final {
 public:
  FeedsChannel(alpha::AsyncTcpClient* async_tcp_client,
               alpha::AsyncTcpConnectionCoroutine* co,
               const alpha::NetAddress& address);
  ~FeedsChannel();

  void WaitAllFeedsSended();
  void AddFightMessage(unsigned msg_type, const google::protobuf::Message* m);

 private:
  static const int kReconnectInterval = 3000;  // ms
  void SendToRemote(NetSvrdFrame::UniquePtr&& frame);
  void ReconnectToRemote();

  alpha::AsyncTcpClient* async_tcp_client_;
  alpha::AsyncTcpConnectionCoroutine* co_;
  alpha::NetAddress address_;
  std::shared_ptr<alpha::AsyncTcpConnection> conn_;
  alpha::UDPSocket socket_;
};
}
