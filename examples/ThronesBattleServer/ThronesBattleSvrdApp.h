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
#include <alpha/SimpleHTTPServer.h>
#include <alpha/experimental/RegionBasedHashMap.h>
#include <alpha/experimental/PidFile.h>
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
  int Init(int argc, char* argv[]);
  int Run();

 private:
  using WarriorMap = alpha::RegionBasedHashMap<UinType, Warrior>;
  using RewardMap = alpha::RegionBasedHashMap<UinType, WarriorLite>;
  static const size_t kRankDataRegionSize;
  static const size_t kRankDataPaddingSize;
  static const size_t kRankMax;
  static const char kBattleDataKey[];
  static const char kWarriorsDataKey[];
  static const char kRewardsDataKey[];
  static const char kRankDataKey[];
  int InitNormalMode();
  int InitRecoveryMode(const char* server_id, const char* suffix);
  void TrapSignals();
  void RoundBattleRoutine(alpha::AsyncTcpClient* client,
                          alpha::AsyncTcpConnectionCoroutine* co, Zone* zone,
                          CampID one, CampID the_other);

  // 备份/恢复
  std::string CreateBackupKey(alpha::Slice key, const char* suffix,
                              const char* server_id = nullptr);
  void BackupRoutine(alpha::AsyncTcpClient* client,
                     alpha::AsyncTcpConnectionCoroutine* co,
                     bool check_last_backup_time = false);
  void RecoveryRoutine(alpha::AsyncTcpClient* client,
                       alpha::AsyncTcpConnectionCoroutine* co,
                       const char* server_id, const char* suffix);
  bool RecoverOneFile(alpha::AsyncTcpConnection* conn,
                      const std::string& backup_key, alpha::MMapFile* file);
  void AddRoundReward(Zone* zone, Camp* camp);

  void ProcessFightTaskResult(BattleContext* ctx,
                              const FightServerProtocol::TaskResult& result);
  void ProcessSurvivedWarrior(BattleContext* ctx, UinType winner,
                              UinType loser);
  void ProcessDeadWarrior(BattleContext* ctx, UinType loser, UinType winner,
                          const std::string& fight_content);
  void ProcessRoundSurvivedWarrior(BattleContext* ctx, UinType winner);

  void DoWhenSeasonChanged();
  void DoWhenSeasonFinished();
  void DoWhenRoundFinished();
  void ReportKillingNumToRank(uint16_t zone_id, UinType uin,
                              uint32_t killing_num);
  void DoWhenTwoCampsMatchDone(BattleContext* ctx);
  void WriteRankFeedsIfSeasonFinished(BattleContext* ctx, Camp* camp);

  void AddTimerForChangeSeason();
  void AddTimerForBattleRound();
  void AddTimerForBackup();
  void InitBeforeNewSeasonBattle();
  void RunBattle();
  std::vector<BattleTask> GetAllUnfinishedTasks();
  void RunBattleTask(BattleTask task);
  void StartAllZonesToCurrentRound();
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
  int HandleQueryWarriorRank(UinType uin, const QueryWarriorRankRequest* req,
                             QueryWarriorRankResponse* resp);

  // Handlers for HTTP message admin
  void HandleHTTPMessage(alpha::TcpConnectionPtr conn,
                         const alpha::HTTPMessage& message);
  alpha::EventLoop loop_;
  std::unique_ptr<ServerConf> conf_;
  std::unique_ptr<alpha::MMapFile> battle_data_underlying_file_;
  std::unique_ptr<alpha::MMapFile> warriors_data_underlying_file_;
  std::unique_ptr<alpha::MMapFile> rewards_data_underlying_file_;
  std::unique_ptr<alpha::MMapFile> rank_data_underlying_file_;
  std::unique_ptr<BattleData> battle_data_;
  std::unique_ptr<WarriorMap> warriors_;
  std::unique_ptr<RewardMap> rewards_;
  std::unique_ptr<alpha::PidFile> pid_file_;
  std::map<uint16_t, std::unique_ptr<RankVector>> season_ranks_;
  std::map<uint16_t, std::unique_ptr<RankVector>> history_ranks_;

  uint64_t server_id_{0};
  bool is_backup_in_progress_{false};
  alpha::TimeStamp last_backup_time_{0};
  alpha::AsyncTcpClient async_tcp_client_;
  MessageDispatcher message_dispatcher_;
  alpha::UDPServer udp_server_;
  alpha::SimpleHTTPServer http_server_;
};
}
