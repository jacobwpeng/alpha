/*
 * =============================================================================
 *
 *       Filename:  ThronesBattleSvrdTaskBroker.cc
 *        Created:  12/15/15 17:31:42
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "ThronesBattleSvrdTaskBroker.h"
#include <alpha/logger.h>
#include <alpha/AsyncTcpConnection.h>
#include <alpha/AsyncTcpConnectionException.h>

namespace ThronesBattle {
static const int kMaxIdleTime = 3000;  // ms
TaskBroker::TaskBroker(alpha::AsyncTcpClient* client,
                       alpha::AsyncTcpConnectionCoroutine* co,
                       const UinList& one_camp_warriors,
                       const UinList& another_camp_warriors,
                       const TaskCallback& cb)
    : client_(client),
      co_(co),
      next_task_id_(1),
      one_camp_warriors_(one_camp_warriors),
      another_camp_warriors_(another_camp_warriors),
      cb_(cb) {
  CHECK(!one_camp_warriors.empty());
  CHECK(one_camp_warriors.size() == another_camp_warriors.size());
}

void TaskBroker::Wait() {
  ReconnectToRemote();
  auto index = 0u;
  while (1) {
    try {
      SendNonAckedTask();
      for (; index < one_camp_warriors_.size(); ++index) {
        FightServerProtocol::Task task;
        auto challenger = one_camp_warriors_[index];
        auto defender = another_camp_warriors_[index];
        auto fight_pair = task.add_fight_pair();
        fight_pair->set_challenger(challenger);
        fight_pair->set_defender(defender);
        AddTask(task);
      }
      WaitAllTasks();
      break;
    } catch (alpha::AsyncTcpConnectionException& e) {
      LOG_WARNING << "TaskBroker::Wait failed, " << e.what();
      ReconnectToRemote();
    }
  }
}

void TaskBroker::SendNonAckedTask(size_t max) {
  size_t sent = 0;
  for (const auto& p : non_acked_tasks_) {
    if (sent < max) {
      SendTaskToRemote(p.second);
    } else {
      break;
    }
  }
}

void TaskBroker::WaitAllTasks() { HandleIncomingData(kMaxIdleTime, true); }

void TaskBroker::AddTask(FightServerProtocol::Task& task) {
  if (non_acked_tasks_.size() == kMaxNonAckedTaskNum) {
    HandleNonAckedTask(false);
  }
  CHECK(non_acked_tasks_.size() < kMaxNonAckedTaskNum);
  auto task_id = next_task_id_;
  ++next_task_id_;
  task.set_fight_type(34);
  task.set_context(task_id);
  auto p = non_acked_tasks_.emplace(task_id, task);
  CHECK(p.second);
  SendTaskToRemote(task);
  HandleCachedReplyData();
}

void TaskBroker::SendTaskToRemote(const FightServerProtocol::Task& task) {
  auto payload_size = task.ByteSize();
  std::unique_ptr<NetSvrdFrame> frame(new (payload_size) NetSvrdFrame);
  bool ok = task.SerializeToArray(frame->payload, frame->payload_size);
  CHECK(ok);
  conn_->Write(frame->data(), frame->size());
  DLOG_INFO << "Send task, id: " << task.context();
}

bool TaskBroker::HandleReplyFrame(const NetSvrdFrame* frame) {
  FightServerProtocol::TaskResult result;
  bool ok = result.ParseFromArray(frame->payload, frame->payload_size);
  if (!ok) {
    LOG_WARNING << "TaskResult::ParseFromArray failed, payload size: "
                << frame->payload_size;
    return false;
  }
  auto task_id = result.context();
  if (task_id >= next_task_id_) {
    LOG_WARNING << "Invalid task id from reply frame context, task id: "
                << task_id;
    return false;
  }
  auto num = non_acked_tasks_.erase(task_id);
  if (num == 0) {
    LOG_INFO << "Maybe multiple reply for this task, task_id: " << task_id;
    return false;
  }
  // Call this only once
  cb_(result);
  return true;
}

void TaskBroker::HandleCachedReplyData() {
  const NetSvrdFrame* frame = nullptr;
  do {
    auto cached_data = conn_->PeekCached();
    frame = NetSvrdFrame::TryCast(cached_data.data(), cached_data.size());
    if (frame == nullptr) {
      break;
    }
    HandleReplyFrame(frame);
    // Maybe use DropCached instead
    auto data = conn_->ReadCached(frame->size());
    CHECK(data.size() == frame->size());
  } while (1);
}

void TaskBroker::HandleNonAckedTask(bool all) {
  auto done = [this, all] {
    return non_acked_tasks_.empty() ||
           (!all && non_acked_tasks_.size() < kMaxNonAckedTaskNum);
  };
  if (!done()) {
    // Try to process cached reply first
    HandleCachedReplyData();
  }
  if (done()) {
    return;
  }
  do {
    // Wait reply from remote
    auto received = HandleIncomingData(kMaxIdleTime, all);
    if (done()) {
      break;
    } else {
      // too many non-acked task
      SendNonAckedTask(received);
    }
  } while (1);
}

size_t TaskBroker::HandleIncomingData(int idle_time, bool all) {
  size_t received = 0;
  do {
    auto data = conn_->Read(NetSvrdFrame::kHeaderSize, idle_time);
    auto frame = reinterpret_cast<const NetSvrdFrame*>(data.data());
    if (frame->magic != NetSvrdFrame::kMagic) {
      LOG_WARNING << "Invalid frame magic, expect: " << NetSvrdFrame::kMagic
                  << ", actual: " << frame->magic;
      return received;
    }
    auto payload = conn_->Read(frame->payload_size, idle_time);
    if (payload.size() != frame->payload_size) {
      LOG_WARNING << "Connection closed by peer, abort this connection";
      return received;
    }
    data.append(payload);
    frame = NetSvrdFrame::TryCast(data.data(), data.size());
    CHECK(frame);
    bool ok = HandleReplyFrame(frame);
    if (ok) received += 1;
    if (non_acked_tasks_.empty()) break;
  } while (all);
  return received;
}

void TaskBroker::ReconnectToRemote() {
  if (conn_) {
    conn_->Close();
    conn_ = nullptr;
  }
  do {
    conn_ = client_->ConnectTo(alpha::NetAddress("10.6.224.83", 443), co_);
    if (conn_ == nullptr) {
      LOG_WARNING << "Failed connect to remote";
      // Yield some time
      co_->YieldWithTimeout(3000);
    }
  } while (conn_ == nullptr);
}
}
