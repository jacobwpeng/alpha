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
#include "fightsvrd.pb.h"
#include "netsvrd_frame.h"

namespace ThronesBattle {
using UinList = std::vector<uint32_t>;
class TaskBroker final {
 public:
  using TaskCallback =
      std::function<void(const FightServerProtocol::TaskResult&)>;
  TaskBroker(alpha::AsyncTcpClient* client,
             alpha::AsyncTcpConnectionCoroutine* co,
             const UinList& one_camp_warriors,
             const UinList& another_camp_warriors, const TaskCallback& cb);
  void Wait();

 private:
  using TaskID = uint64_t;
  static const size_t kMaxNonAckedTaskNum = 256;
  void AddTask(FightServerProtocol::Task& task);
  void HandleCachedReplyData();
  void SendTaskToRemote(const FightServerProtocol::Task& task);
  bool HandleReplyFrame(const NetSvrdFrame* frame);
  void HandleNonAckedTask(bool all);
  size_t HandleIncomingData(int idle_time, bool all);

  void SendNonAckedTask(size_t max = std::numeric_limits<size_t>::max());
  void WaitAllTasks();
  void ReconnectToRemote();

  alpha::AsyncTcpClient* client_;
  alpha::AsyncTcpConnectionCoroutine* co_;
  std::shared_ptr<alpha::AsyncTcpConnection> conn_;
  TaskID next_task_id_;
  UinList one_camp_warriors_;
  UinList another_camp_warriors_;
  TaskCallback cb_;
  std::map<TaskID, FightServerProtocol::Task> non_acked_tasks_;
};
}
