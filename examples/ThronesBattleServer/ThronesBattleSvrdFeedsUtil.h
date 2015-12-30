/*
 * =============================================================================
 *
 *       Filename:  ThronesBattleSvrdFeedsUtil.h
 *        Created:  12/29/15 19:42:26
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once
#include <sstream>
#include <alpha/logger.h>
#include <alpha/UDPSocket.h>
#include "ThronesBattleSvrdDef.h"
#include "feedssvrd.pb.h"
#include "netsvrd_frame.h"

namespace ThronesBattle {
static const unsigned kThronesBattleLose = 450;
static const unsigned kThronesBattleWin = 451;
static const unsigned kThronesBattleBye = 452;
static const unsigned kThronesBattleCampFirst = 453;
static const unsigned kThronesBattleCampSecond = 454;
static const unsigned kThronesBattleCampThird = 455;
static const unsigned kThronesBattleCampOthers = 456;
static const unsigned kThronesBattleEventType = 22;
static const unsigned kThronesBattleCampRank[kCampIDMax] = {453, 454, 455, 456,
                                                            456, 456, 456, 456};
static int udp_message_num = 0;
template <typename... Args>
std::string CreateParams(std::ostringstream& oss) {
  return oss.str();
}

template <typename Arg, typename... Tail>
std::string CreateParams(std::ostringstream& oss, Arg&& arg, Tail&&... tail) {
  oss << arg;
  if (sizeof...(tail) >= 1) {
    oss << ':';
  }
  return CreateParams(oss, tail...);
}

template <typename... Args>
void AddFightMessage(alpha::UDPSocket* socket, unsigned msg_type, unsigned self,
                     unsigned opponent, Args&&... args) {
#if 1
  (void)socket;
  std::ostringstream oss;
  std::string params = CreateParams(oss, std::forward<Args&&>(args)...);
  FeedsServerProtocol::FightMessageTask fight_message_task;
  fight_message_task.set_src_uin(self);
  fight_message_task.set_dst_uin(opponent);
  fight_message_task.set_params(params);
  FeedsServerProtocol::Task task;
  task.set_msg_type(msg_type);
  bool ok = fight_message_task.SerializeToString(task.mutable_payload());
  CHECK(ok)
      << "FeedsServerProtocol::FightMessageTask::SerializeToString failed";
  auto frame = NetSvrdFrame::CreateUnique(task.ByteSize());
  ok = task.SerializeToArray(frame->payload, frame->payload_size);
  CHECK(ok) << "FeedsServerProtocol::Task::SerializeToArray failed";
  alpha::IOBufferWithSize buf(frame->size());
  memcpy(buf.data(), frame->data(), frame->size());
  int err = socket->Write(&buf, buf.size());
  if (err < 0) {
    PLOG_ERROR << "UDPSocket::Write failed";
  } else {
    ++udp_message_num;
    DLOG_INFO << "udp_message_num: " << udp_message_num;
  }
#endif
}

template <typename... Args>
void AddFightEvent(alpha::UDPSocket* socket, unsigned msg_type, unsigned self,
                   unsigned opponent, const std::string& reply,
                   Args&&... args) {
#if 1
  std::ostringstream oss;
  std::string params = CreateParams(oss, std::forward<Args&&>(args)...);
  FeedsServerProtocol::FightEventTask fight_event_task;
  fight_event_task.set_src_uin(self);
  fight_event_task.set_dst_uin(opponent);
  fight_event_task.set_event_type(kThronesBattleEventType);
  fight_event_task.set_replay_content(reply);
  fight_event_task.set_params(params);
  FeedsServerProtocol::Task task;
  task.set_msg_type(msg_type);
  bool ok = fight_event_task.SerializeToString(task.mutable_payload());
  CHECK(ok) << "FeedsServerProtocol::FightEventTask::SerializeToString failed";
  auto frame = NetSvrdFrame::CreateUnique(task.ByteSize());
  ok = task.SerializeToArray(frame->payload, frame->payload_size);
  CHECK(ok) << "FeedsServerProtocol::Task::SerializeToArray failed";
  alpha::IOBufferWithSize buf(frame->size());
  memcpy(buf.data(), frame->data(), frame->size());
  int err = socket->Write(&buf, buf.size());
  if (err < 0) {
    PLOG_ERROR << "UDPSocket::Write failed";
  } else {
    ++udp_message_num;
    DLOG_INFO << "udp_message_num: " << udp_message_num;
  }
#endif
}
}
