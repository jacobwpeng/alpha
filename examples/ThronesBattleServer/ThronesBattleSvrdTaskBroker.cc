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
TaskBroker::TaskBroker(alpha::AsyncTcpClient* client,
                       alpha::AsyncTcpConnectionCoroutine* co,
                       const alpha::NetAddress& fight_server_addr,
                       uint16_t zone, uint16_t camp, const TaskCallback& cb)
    : client_(client),
      co_(co),
      fight_server_addr_(fight_server_addr),
      zone_(zone),
      camp_(camp),
      next_task_id_(1),
      cb_(cb) {}

TaskBroker::~TaskBroker() {
  if (conn_) {
    conn_->Close();
  }
}

void TaskBroker::Wait() {
  ConnectToRemote();
  CHECK(one_camp_warriors_.size() == the_other_camp_warriors_.size());
  size_t index = 0;
  while (1) {
    try {
      SendNonAckedTask();
      for (; index < one_camp_warriors_.size(); ++index) {
        FightServerProtocol::Task task;
        auto challenger = one_camp_warriors_[index];
        auto defender = the_other_camp_warriors_[index];
        auto fight_pair = task.add_fight_pair();
        fight_pair->set_challenger(challenger);
        fight_pair->set_defender(defender);
        AddTask(task);
      }
      WaitAllTasks();
      if (non_acked_tasks_.empty()) break;
    } catch (alpha::AsyncTcpConnectionOperationTimeout& e) {
      LOG_WARNING << "Operation timeout, try reestablish connection";
      ReconnectToRemote();
    } catch (alpha::AsyncTcpConnectionException& e) {
      LOG_WARNING << "TaskBroker::Wait failed, " << e.what();
      ReconnectToRemote();
      DLOG_INFO << "After ReconnectToRemote, index = " << index
                << ", one_camp_warriors_.size() = " << one_camp_warriors_.size()
                << ", non_acked_tasks_.size() = " << non_acked_tasks_.size();
    }
  }
}

void TaskBroker::SendNonAckedTask(size_t max) {
  if (non_acked_tasks_.empty()) {
    return;
  }
  if (max == std::numeric_limits<size_t>::max()) {
    LOG_INFO << "Send all non acked task(s), current: "
             << non_acked_tasks_.size();
  } else {
    LOG_INFO << "Send at most " << max << " non acked task(s)";
  }
  size_t sent = 0;
  for (auto it = non_acked_tasks_.begin();
       it != non_acked_tasks_.end() && sent < max; ++sent, ++it) {
    SendTaskToRemote(it->second);
  }
}

void TaskBroker::WaitAllTasks() { HandleIncomingData(kMaxIdleTime, true); }

void TaskBroker::AddTask(FightServerProtocol::Task& task) {
  if (non_acked_tasks_.size() == kMaxNonAckedTaskNum) {
    HandleNonAckedTask(false);
  }
  static const int kThronesBattleFightType = 34;
  CHECK(non_acked_tasks_.size() < kMaxNonAckedTaskNum);
  auto task_id = next_task_id_;
  ++next_task_id_;
  task.set_fight_type(kThronesBattleFightType);
  task.set_context(task_id);
  auto p = non_acked_tasks_.emplace(task_id, task);
  CHECK(p.second);
  SendTaskToRemote(task);
  HandleCachedReplyData();
}

void TaskBroker::SendTaskToRemote(const FightServerProtocol::Task& task) {
  auto payload_size = task.ByteSize();
  auto frame = NetSvrdFrame::CreateUnique(payload_size);
  bool ok = task.SerializeToArray(frame->payload, frame->payload_size);
  CHECK(ok);
  conn_->Write(frame->data(), frame->size());
  DLOG_INFO << "(" << zone_ << ", " << camp_ << "), "
            << "Send task, id: " << task.context();
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
    DLOG_INFO << "(" << zone_ << ", " << camp_ << "), "
              << "Maybe multiple reply for this task, task_id: " << task_id;
    return false;
  }
  DLOG_INFO << "Handled one reply frame, task_id: " << task_id;
  // Call this only once
  cb_(result);
  return true;
}

void TaskBroker::HandleCachedReplyData() {
  do {
    size_t length;
    auto cached_data = conn_->PeekCached(&length);
    auto header = NetSvrdFrame::CastHeaderOnly(cached_data, length);
    if (header == nullptr) {
      break;
    }
    if (length < header->size()) {
      break;
    }
    auto frame = reinterpret_cast<const NetSvrdFrame*>(cached_data);
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
  if (non_acked_tasks_.empty()) {
    return 0;
  }
  size_t received = 0;
  do {
    auto data = conn_->Read(NetSvrdFrame::kHeaderSize, idle_time);
    auto header = NetSvrdFrame::CastHeaderOnly(data.data(), data.size());
    if (header == nullptr) {
      LOG_WARNING << "Cast NetSvrdFrame header failed";
      // TODO: 关闭连接
      return received;
    }
    auto payload = conn_->Read(header->payload_size, idle_time);
    CHECK(payload.size() == header->payload_size);
    data.append(payload);
    auto frame = reinterpret_cast<const NetSvrdFrame*>(data.data());
    bool ok = HandleReplyFrame(frame);
    if (ok) received += 1;
    if (non_acked_tasks_.empty()) {
      break;
    }
  } while (all);
  return received;
}

void TaskBroker::ConnectToRemote() {
  CHECK(conn_ == nullptr);
  do {
    DLOG_INFO << "Connecting to fight server " << fight_server_addr_;
    conn_ = client_->ConnectTo(fight_server_addr_, co_);
    if (conn_) {
      DLOG_INFO << "Connected to fight server " << fight_server_addr_;
      break;
    }
    static const int kRetryInterval = 3000;
    LOG_WARNING << "Failed connect to fight server, retry after "
                << kRetryInterval << " ms";
    co_->YieldWithTimeout(kRetryInterval);
  } while (conn_ == nullptr);
}

void TaskBroker::ReconnectToRemote() {
  if (conn_) {
    conn_->Close();
    conn_ = nullptr;
  }
  ConnectToRemote();
  LOG_INFO_IF(conn_) << "Reconnect succeed";
}
}
