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
#include <alpha/UDPSocket.h>
#include <alpha/UDPServer.h>
#include <alpha/experimental/RegionBasedHashMap.h>
#include "ThronesBattleSvrdDef.h"
#include "ThronesBattleSvrdMessageDispatcher.h"
#include "ThronesBattleSvrdRankVector.h"

// 使用到的PB结构的前置声明
namespace FightServerProtocol {
class TaskResult;
}
namespace ThronesBattleServerProtocol {
class Goods;
class Reward;
class General;
class CampInfo;
class RoundMatchups;
class Matchups;
class LuckyWarriors;
class RequestWrapper;
class ResponseWrapper;
class SignUpRequest;
class SignUpResponse;
class QueryBattleStatusRequest;
class QueryBattleStatusResponse;
class PickLuckyWarriorsRequest;
class PickLuckyWarriorsResponse;
class QueryLuckyWarriorRewardRequest;
class QueryLuckyWarriorRewardResponse;
class QueryGeneralInChiefRequest;
class QueryGeneralInChiefResponse;
}

using namespace ThronesBattleServerProtocol;
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
  using RewardMap = alpha::RegionBasedHashMap<UinType, WarriorLite>;
  static const size_t kRankDataRegionSize;
  static const size_t kRankDataPaddingSize;
  static const size_t kRankMax;
  int InitNormalMode();
  int InitRecoveryMode();
  void TrapSignals();
  void RoundBattleRoutine(alpha::AsyncTcpClient* client,
                          alpha::AsyncTcpConnectionCoroutine* co, Zone* zone,
                          CampID one, CampID the_other);
  void BackupRoutine(alpha::AsyncTcpClient* client,
                     alpha::AsyncTcpConnectionCoroutine* co,
                     bool check_last_backup_time = false);
  void AddRoundReward(Zone* zone, Camp* camp);

  void ProcessFightTaskResult(const FightServerProtocol::TaskResult& result);
  void ProcessSurvivedWarrior(UinType winner, UinType loser);
  void ProcessDeadWarrior(UinType loser, UinType winner,
                          const std::string& fight_content);
  void ProcessRoundSurvivedWarrior(UinType winner);

  void DoWhenSeasonChanged();
  void DoWhenSeasonFinished();
  void DoWhenRoundFinished();
  void DoWhenTwoCampsMatchDone(Zone* zone, Camp* one, Camp* the_other,
                               Camp* winner_camp);
  void ReportKillingNum(uint16_t zone_id, UinType uin);

  void AddTimerForChangeSeason();
  void AddTimerForBattleRound();
  void AddTimerForBackup();
  void InitBeforeNewSeasonBattle();
  void RunBattle();
  bool RecoveryMode() const;

  // Handlers for CGI UDP requests
  void HandleUDPMessage(alpha::UDPSocket* socket, alpha::IOBuffer* buf,
                        size_t buf_len, const alpha::NetAddress& peer);
  int HandleQuerySignUp(UinType uin, const QuerySignUpRequest* req,
                        QuerySignUpResponse* resp);
  int HandleSignUp(UinType uin, const SignUpRequest* req, SignUpResponse* resp);
  int HandleQueryBattleStatus(UinType uin, const QueryBattleStatusRequest* req,
                              QueryBattleStatusResponse* resp);
  int HandlePickLuckyWarriors(UinType uin, const PickLuckyWarriorsRequest* req,
                              PickLuckyWarriorsResponse* resp);
  int HandleQueryLuckyWarriorReward(UinType uin,
                                    const QueryLuckyWarriorRewardRequest* req,
                                    QueryLuckyWarriorRewardResponse* resp);
  int HandleQueryRoundRewardRequest(UinType uin,
                                    const QueryRoundRewardRequest* req,
                                    QueryRoundRewardResponse* resp);
  int HandleQueryGeneralInChief(UinType uin,
                                const QueryGeneralInChiefRequest* req,
                                QueryGeneralInChiefResponse* resp);
  int HandleQueryRank(UinType uin, const QueryRankRequest* req,
                      QueryRankResponse* resp);
  alpha::EventLoop loop_;
  std::unique_ptr<ServerConf> conf_;
  std::unique_ptr<alpha::MMapFile> battle_data_underlying_file_;
  std::unique_ptr<alpha::MMapFile> warriors_data_underlying_file_;
  std::unique_ptr<alpha::MMapFile> rewards_data_underlying_file_;
  std::unique_ptr<alpha::MMapFile> rank_data_underlying_file_;
  std::unique_ptr<BattleData> battle_data_;
  std::unique_ptr<WarriorMap> warriors_;
  std::unique_ptr<RewardMap> rewards_;
  std::map<uint16_t, std::unique_ptr<RankVector>> season_ranks_;
  std::map<uint16_t, std::unique_ptr<RankVector>> history_ranks_;

  uint64_t server_id_{0};
  bool is_backup_in_progress_{false};
  alpha::TimeStamp last_backup_time_{0};
  alpha::AsyncTcpClient async_tcp_client_;
  MessageDispatcher message_dispatcher_;
  alpha::UDPServer udp_server_;
  alpha::UDPSocket feeds_socket_;
  alpha::UDPSocket rank_socket_;
};
}
