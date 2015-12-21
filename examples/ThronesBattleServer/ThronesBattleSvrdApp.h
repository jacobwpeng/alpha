/*
 * =============================================================================
 *
 *       Filename:  ThronesBattleSvrdApp.h
 *        Created:  12/16/15 17:02:33
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include <memory>
#include <alpha/event_loop.h>
#include <alpha/MMapFile.h>
#include <alpha/AsyncTcpClient.h>
#include <alpha/experimental/RegionBasedHashMap.h>
#include "ThronesBattleSvrdDef.h"

namespace ThronesBattle {
class ServerConf;
class ServerApp final {
 public:
  ServerApp();
  ~ServerApp();
  int Init(const char* file);
  int Run();

 private:
  using WarriorMap = alpha::RegionBasedHashMap<UinType, Warrior>;
  using RewardMap = alpha::RegionBasedHashMap<UinType, Reward>;
  int InitNormalMode();
  int InitRemoveryMode();
  void TrapSignals();
  void RoundBattleRoutine(alpha::AsyncTcpClient* client,
                          alpha::AsyncTcpConnectionCoroutine* co, Zone* zone,
                          CampID one, CampID the_other);
  void MarkWarriorDead(UinType uin);

  void AddTimerForChangeSeason();
  void AddTimerForDropLastSeasonData();
  void AddTimerForBattleRound();
  void RunBattle();
  bool RecoveryMode() const;
  alpha::EventLoop loop_;
  std::unique_ptr<ServerConf> conf_;
  std::unique_ptr<alpha::MMapFile> battle_data_underlying_file_;
  std::unique_ptr<alpha::MMapFile> warriors_data_underlying_file_;
  std::unique_ptr<alpha::MMapFile> rewards_data_underlying_file_;
  std::unique_ptr<BattleData> battle_data_;
  std::unique_ptr<WarriorMap> warriors_;
  std::unique_ptr<RewardMap> rewards_;

  alpha::AsyncTcpClient async_tcp_client_;
};
}
