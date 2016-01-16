/*
 * =============================================================================
 *
 *       Filename:  ThronesBattleSvrdTaskBroker.h
 *        Created:  12/15/15 17:28:35
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include <map>
#include <vector>
#include <functional>
#include <alpha/AsyncTcpClient.h>
#include "ThronesBattleSvrdDef.h"
#include "proto/fightsvrd.pb.h"
#include "ext/netsvrd_frame.h"

namespace ThronesBattle {
class TaskBroker final {
 public:
  using TaskCallback =
      std::function<void(const FightServerProtocol::TaskResult&)>;
  TaskBroker(alpha::AsyncTcpClient* client,
             alpha::AsyncTcpConnectionCoroutine* co,
             const alpha::NetAddress& fight_server_addr, uint16_t zone,
             uint16_t camp, const TaskCallback& cb);
  ~TaskBroker();

  void SetOneCampWarriorRange(const UinList& uins) {
    one_camp_warriors_ = uins;
  }
  void SetTheOtherCampWarriorRange(const UinList& uins) {
    the_other_camp_warriors_ = uins;
  }
  void Wait();

 private:
  using TaskID = uint64_t;
  static const size_t kMaxNonAckedTaskNum = 32;
  static const int kMaxIdleTime = 10000;  // ms
  void AddTask(FightServerProtocol::Task& task);
  void HandleCachedReplyData();
  void SendTaskToRemote(const FightServerProtocol::Task& task);
  bool HandleReplyFrame(const NetSvrdFrame* frame);
  void HandleNonAckedTask(bool all);
  size_t HandleIncomingData(int idle_time, bool all);

  void SendNonAckedTask(size_t max = std::numeric_limits<size_t>::max());
  void WaitAllTasks();
  void ConnectToRemote();
  void ReconnectToRemote();

  alpha::AsyncTcpClient* client_;
  alpha::AsyncTcpConnectionCoroutine* co_;
  std::shared_ptr<alpha::AsyncTcpConnection> conn_;
  alpha::NetAddress fight_server_addr_;
  uint16_t zone_;
  uint16_t camp_;
  TaskID next_task_id_;
  UinList one_camp_warriors_;
  UinList the_other_camp_warriors_;
  TaskCallback cb_;
  std::map<TaskID, FightServerProtocol::Task> non_acked_tasks_;
};
}
