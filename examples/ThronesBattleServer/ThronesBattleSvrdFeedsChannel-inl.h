/*
 * =============================================================================
 *
 *       Filename:  ThronesBattleSvrdFeedsChannel-inl.h
 *        Created:  01/05/16 16:50:09
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "proto/feedssvrd.pb.h"
#include <alpha/logger.h>

namespace ThronesBattle {
template <typename Arg, typename... Tail>
std::string FeedsChannel::CreateParams(std::ostringstream& oss, Arg&& arg,
                                       Tail&&... tail) {
  const char kParamSep = ':';
  oss << arg;
  if (sizeof...(tail) != 0) {
    oss << kParamSep;
  }
  return CreateParams(oss, tail...);
}

template <typename... Args>
void FeedsChannel::AddFightMessage(unsigned msg_type, unsigned self,
                                   unsigned opponent, Args&&... args) {
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
  SendToRemote(std::move(frame));
}

template <typename... Args>
void FeedsChannel::AddFightEvent(unsigned msg_type, unsigned self,
                                 unsigned opponent, const std::string& reply,
                                 Args&&... args) {
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
  SendToRemote(std::move(frame));
}
}
