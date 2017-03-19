/*
 * =============================================================================
 *
 *       Filename:  ThronesBattleSvrdApp.cc
 *        Created:  12/16/15 17:03:19
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "ThronesBattleSvrdApp.h"
#include <unistd.h>
#include <boost/algorithm/string/predicate.hpp>
#include <google/protobuf/descriptor.h>
#include <alpha/Format.h>
#include <alpha/Logger.h>
#include <alpha/Random.h>
#include <alpha/Endian.h>
#include <alpha/IOBuffer.h>
#include <alpha/FileUtil.h>
#include <alpha/AsyncTcpConnection.h>
#include <alpha/AsyncTcpConnectionException.h>
#include <alpha/AsyncTcpConnectionCoroutine.h>
#include "ThronesBattleSvrdConf.h"
#include "ThronesBattleSvrdTaskBroker.h"
#include "ThronesBattleSvrdFeedsChannel.h"
#include "proto/ThronesBattleSvrd.pb.h"
#include "proto/feedssvrd.pb.h"
#include "proto/fightsvrd.pb.h"

using namespace std::placeholders;
namespace ThronesBattle {
namespace detail {
std::unique_ptr<alpha::MMapFile> OpenMemoryMapedFile(alpha::Slice path,
                                                     uint32_t size) {
  auto open_flags = alpha::MMapFile::kCreateIfNotExists;
  auto file = alpha::MMapFile::Open(path, size, open_flags);
  if (file == nullptr) {
    LOG_ERROR << "Open memory maped file failed, path: " << path.ToString();
    return nullptr;
  }
  return std::move(file);
}

void CopyMemoryMapedFileToMemory(alpha::MMapFile* file,
                                 alpha::IOBufferWithSize* buf) {
  CHECK(file && buf);
  CHECK(buf->size() == file->size());
  memcpy(buf->data(), file->start(), file->size());
}

template <typename T>
std::unique_ptr<T> MakeFromMemoryMapedFile(alpha::MMapFile* file) {
  const char* op;
  std::unique_ptr<T> result;
  char* start = reinterpret_cast<char*>(file->start());
  if (file->newly_created()) {
    op = "Create";
    result = T::Create(start, file->size());
  } else {
    op = "Restore";
    result = T::Restore(start, file->size());
  }
  if (result == nullptr) {
    LOG_ERROR << op << " data from file failed";
    return nullptr;
  }
  return std::move(result);
}
}

const size_t ServerApp::kRankDataRegionSize = 1 << 16;
const size_t ServerApp::kRankDataPaddingSize = 1 << 14;
const size_t ServerApp::kRankMax = 1000;
const char ServerApp::kBattleDataKey[] = "BattleData";
const char ServerApp::kWarriorsDataKey[] = "WarriorsData";
const char ServerApp::kRewardsDataKey[] = "RewardsData";
const char ServerApp::kRankDataKey[] = "RankData";

ServerApp::ServerApp()
    : async_tcp_client_(&loop_), udp_server_(&loop_), http_server_(&loop_) {}

ServerApp::~ServerApp() {
  if (pid_file_) {
    alpha::DeleteFile(conf_->pid_file());
  }
}

int ServerApp::Init(int argc, char* argv[]) {
  CHECK(argc == 2 || argc == 4) << "Invalid argc: " << argc;
  const char* file = argv[1];
  server_id_ = alpha::Random::Rand64();
  conf_ = ServerConf::Create(file);
  if (conf_ == nullptr) {
    return EXIT_FAILURE;
  }

  LOG_INFO << "Battle data file: " << conf_->battle_data_file();
  LOG_INFO << "Battle data file size(Byte): " << conf_->battle_data_file_size();
  LOG_INFO << "Warriors data file: " << conf_->warriors_data_file();
  LOG_INFO << "Warriors data file size(Byte): "
           << conf_->warriors_data_file_size();
  LOG_INFO << "Rewards data file: " << conf_->rewards_data_file();
  LOG_INFO << "Rewards data file size(Byte): "
           << conf_->rewards_data_file_size();
  LOG_INFO << "Rank data file: " << conf_->rank_data_file();
  LOG_INFO << "Rank data file size(Byte): " << conf_->rank_data_file_size();
  LOG_INFO << "Fight server addr: " << conf_->fight_server_addr();
  LOG_INFO << "Feeds server addr: " << conf_->feeds_server_addr();
  LOG_INFO << "Backup server addr: " << conf_->backup_server_addr();
  LOG_INFO << "Backup interval(seconds): " << conf_->backup_interval();

  battle_data_underlying_file_ = detail::OpenMemoryMapedFile(
      conf_->battle_data_file(), conf_->battle_data_file_size());
  if (battle_data_underlying_file_ == nullptr) {
    return EXIT_FAILURE;
  }

  warriors_data_underlying_file_ = detail::OpenMemoryMapedFile(
      conf_->warriors_data_file(), conf_->warriors_data_file_size());
  if (warriors_data_underlying_file_ == nullptr) {
    return EXIT_FAILURE;
  }

  rewards_data_underlying_file_ = detail::OpenMemoryMapedFile(
      conf_->rewards_data_file(), conf_->rewards_data_file_size());
  if (rewards_data_underlying_file_ == nullptr) {
    return EXIT_FAILURE;
  }

  rank_data_underlying_file_ = detail::OpenMemoryMapedFile(
      conf_->rank_data_file(), conf_->rank_data_file_size());
  if (rank_data_underlying_file_ == nullptr) {
    return EXIT_FAILURE;
  }

  return argc == 4 ? InitRecoveryMode(argv[2], argv[3]) : InitNormalMode();
}

int ServerApp::InitNormalMode() {
  const char* op;
  BattleDataSaved* battle_data_saved = nullptr;
  if (battle_data_underlying_file_->newly_created()) {
    op = "Create";
    battle_data_saved =
        BattleDataSaved::Create(battle_data_underlying_file_->start(),
                                battle_data_underlying_file_->size());
  } else {
    op = "Restore";
    battle_data_saved =
        BattleDataSaved::Restore(battle_data_underlying_file_->start(),
                                 battle_data_underlying_file_->size());
  }
  if (battle_data_saved == nullptr) {
    LOG_ERROR << op << " data from battle data file failed";
    return EXIT_FAILURE;
  }

  warriors_ = detail::MakeFromMemoryMapedFile<WarriorMap>(
      warriors_data_underlying_file_.get());
  if (warriors_ == nullptr) {
    LOG_ERROR << "Make warriors data failed";
    return EXIT_FAILURE;
  }
  LOG_INFO << "WarriorMap capacity: " << warriors_->max_size()
           << ", current size: " << warriors_->size();

  rewards_ = detail::MakeFromMemoryMapedFile<RewardMap>(
      rewards_data_underlying_file_.get());

  if (rewards_ == nullptr) {
    LOG_ERROR << "Make rewards data failed";
    return EXIT_FAILURE;
  }
  LOG_INFO << "RewardMap capacity: " << rewards_->max_size()
           << ", current size: " << rewards_->size();
  if (rewards_->max_size() < warriors_->max_size()) {
    LOG_ERROR << "RewardMap's capacity should large than WarriorMap's capacity";
    return EXIT_FAILURE;
  }

  if (rank_data_underlying_file_->size() <
      2 * kMaxZoneNum * (kRankDataRegionSize + kRankDataPaddingSize)) {
    LOG_ERROR << "Rank data file size is too small";
    return EXIT_FAILURE;
  }

  // 恢复本届排行榜
  size_t offset = 0;
  auto const file_start =
      reinterpret_cast<char*>(rank_data_underlying_file_->start());
  for (uint16_t i = 0; i < kMaxZoneNum; ++i) {
    auto zone_id = i + 1;
    CHECK(offset + kRankDataRegionSize < rank_data_underlying_file_->size())
        << "Invalid offset: " << offset
        << ", file size: " << rank_data_underlying_file_->size();
    auto data = file_start + offset;
    auto rank = alpha::make_unique<RankVector>(kRankMax);
    CHECK(rank);
    CHECK(data + kRankDataRegionSize <=
          file_start + rank_data_underlying_file_->size());
    if (rank_data_underlying_file_->newly_created()) {
      bool ok = rank->CreateFrom(data, kRankDataRegionSize);
      if (!ok) {
        LOG_ERROR << "Create rank failed, zone: " << zone_id;
        return EXIT_FAILURE;
      }
    } else {
      bool ok = rank->RestoreFrom(data, kRankDataRegionSize);
      if (!ok) {
        LOG_ERROR << "Restore rank failed, zone: " << zone_id;
        return EXIT_FAILURE;
      }
    }
    season_ranks_[zone_id] = std::move(rank);
    offset += kRankDataRegionSize + kRankDataPaddingSize;
  }

  // 恢复上届排行榜
  for (uint16_t i = 0; i < kMaxZoneNum; ++i) {
    auto zone_id = i + 1;
    auto data = file_start + offset;
    auto rank = alpha::make_unique<RankVector>(kRankMax);
    CHECK(rank);
    CHECK(offset + kRankDataRegionSize < rank_data_underlying_file_->size())
        << "Invalid offset: " << offset
        << ", file size: " << rank_data_underlying_file_->size();
    if (rank_data_underlying_file_->newly_created()) {
      bool ok = rank->CreateFrom(data, kRankDataRegionSize);
      if (!ok) {
        LOG_ERROR << "Create rank failed, zone: " << zone_id;
        return EXIT_FAILURE;
      }
    } else {
      bool ok = rank->RestoreFrom(data, kRankDataRegionSize);
      if (!ok) {
        LOG_ERROR << "Restore rank failed, zone: " << zone_id;
        return EXIT_FAILURE;
      }
    }
    history_ranks_[zone_id] = std::move(rank);
    offset += kRankDataRegionSize + kRankDataPaddingSize;
  }

  battle_data_ = alpha::make_unique<BattleData>(battle_data_saved);
  for (const auto& p : *warriors_) {
    auto& warrior = p.second;
    auto zone_id = warrior.zone_id();
    auto camp_id = warrior.camp_id();
    auto zone = battle_data_->GetZone(zone_id);
    auto camp = zone->GetCamp(camp_id);
    auto dead = warrior.dead();
    camp->AddWarrior(warrior.uin(), dead);
  }

  http_server_.SetCallback(
      std::bind(&ServerApp::HandleHTTPMessage, this, _1, _2));

#define THRONES_BATTLE_REGISTER_HANDLER(ReqType, RespType, Handler) \
  message_dispatcher_.Register<ReqType, RespType>(                  \
      std::bind(&ServerApp::Handler, this, _1, _2, _3));

  THRONES_BATTLE_REGISTER_HANDLER(
      QuerySignUpRequest, QuerySignUpResponse, HandleQuerySignUp);
  THRONES_BATTLE_REGISTER_HANDLER(SignUpRequest, SignUpResponse, HandleSignUp);
  THRONES_BATTLE_REGISTER_HANDLER(QueryBattleStatusRequest,
                                  QueryBattleStatusResponse,
                                  HandleQueryBattleStatus);
  THRONES_BATTLE_REGISTER_HANDLER(PickLuckyWarriorsRequest,
                                  PickLuckyWarriorsResponse,
                                  HandlePickLuckyWarriors);
  THRONES_BATTLE_REGISTER_HANDLER(QueryLuckyWarriorRewardRequest,
                                  QueryLuckyWarriorRewardResponse,
                                  HandleQueryLuckyWarriorReward);
  THRONES_BATTLE_REGISTER_HANDLER(QueryRoundRewardRequest,
                                  QueryRoundRewardResponse,
                                  HandleQueryRoundRewardRequest);
  THRONES_BATTLE_REGISTER_HANDLER(QueryGeneralInChiefRequest,
                                  QueryGeneralInChiefResponse,
                                  HandleQueryGeneralInChief);
  THRONES_BATTLE_REGISTER_HANDLER(
      QueryRankRequest, QueryRankResponse, HandleQueryRank);
  THRONES_BATTLE_REGISTER_HANDLER(QueryWarriorRankRequest,
                                  QueryWarriorRankResponse,
                                  HandleQueryWarriorRank);
#undef THRONES_BATTLE_REGISTER_HANDLER

  if (battle_data_->SeasonFinished()) {
    AddTimerForChangeSeason();
  } else {
    AddTimerForBattleRound();
  }
  // 保证启停不会马上备份, 如果需要使用HTTP管理发起备份
  last_backup_time_ = alpha::Now();
  AddTimerForBackup();
  return 0;
}

int ServerApp::InitRecoveryMode(const char* server_id, const char* suffix) {
  loop_.QueueInLoop([this, server_id, suffix] {
    async_tcp_client_.RunInCoroutine(std::bind(
        &ServerApp::RecoveryRoutine, this, _1, _2, server_id, suffix));
  });
  return EXIT_SUCCESS;
}

bool ServerApp::CreatePidFile() {
  alpha::File file(conf_->pid_file(), O_WRONLY | O_CREAT);
  if (!file) {
    PLOG_ERROR << "Open pid file failed";
    return false;
  }
  if (!file.TryLock()) {
    PLOG_ERROR << "Lock pid file failed";
    return false;
  }
  auto pid = std::to_string(getpid());
  int rc = file.Write(pid.c_str(), pid.size());
  if (rc != static_cast<int>(pid.size())) {
    PLOG_ERROR << "Write pid to file failed";
    return false;
  }
  pid_file_ = std::move(file);
  return true;
}

void ServerApp::TrapSignals() {
  auto ignore = [] {};
  auto quit = [this] { loop_.Quit(); };

  loop_.TrapSignal(SIGINT, quit);
  loop_.TrapSignal(SIGQUIT, quit);
  loop_.TrapSignal(SIGTERM, quit);

  loop_.TrapSignal(SIGHUP, ignore);
  loop_.TrapSignal(SIGCHLD, ignore);
  loop_.TrapSignal(SIGPIPE, ignore);
}

void ServerApp::RoundBattleRoutine(alpha::AsyncTcpClient* client,
                                   alpha::AsyncTcpConnectionCoroutine* co,
                                   Zone* zone,
                                   CampID one,
                                   CampID the_other) {
  LOG_INFO << "Perform battle between camps, Zone: " << zone->id()
           << ", one camp: " << one << ", the other camp: " << the_other
           << "coroutine id: " << co->id();

  auto one_camp = zone->GetCamp(one);
  auto the_other_camp = zone->GetCamp(the_other);
  auto done = [one_camp, the_other_camp] {
    return one_camp->NoLivingWarriors() || the_other_camp->NoLivingWarriors();
  };

  LOG_INFO << "Before battle, Zone: " << zone->id()
           << ", one camp living: " << one_camp->LivingWarriorsNum()
           << ", the other camp living: "
           << the_other_camp->LivingWarriorsNum();

  FeedsChannel feeds_channel(client, co, conf_->feeds_server_addr());
  BattleContext context = {.async_tcp_client = client,
                           .co = co,
                           .zone = zone,
                           .one = one_camp,
                           .the_other = the_other_camp,
                           .winner = nullptr,
                           .feeds_channel = &feeds_channel};
  BattleContext* ctx = &context;  // 统一各处的ctx->

  if (one_camp->NoLivingWarriors() && the_other_camp->NoLivingWarriors()) {
    // 两个阵营都没人, 随机一个胜利阵营
    ctx->winner = alpha::Random::Rand32(2) ? ctx->one : ctx->the_other;
    LOG_INFO << "No warrior found on both sides, one: " << ctx->one->id()
             << ", the_other: " << ctx->the_other->id()
             << ", random winner: " << ctx->winner->id();
    DoWhenTwoCampsMatchDone(ctx);
    return;
  }

  auto warrior_fight_result_callback =
      std::bind(&ServerApp::ProcessFightTaskResult, this, ctx, _1);

  for (auto match = 1; !done(); ++match) {
    // 随机取出和人数较少一方人数的玩家
    auto battle_warrior_size = std::min(ctx->one->LivingWarriorsNum(),
                                        ctx->the_other->LivingWarriorsNum());
    CHECK(battle_warrior_size != 0u);

    auto one_camp_choosen_warriors =
        ctx->one->RandomChooseLivingWarriors(battle_warrior_size);
    auto the_other_camp_choosen_warriors =
        ctx->the_other->RandomChooseLivingWarriors(battle_warrior_size);

    // 进行战斗
    TaskBroker broker(client,
                      co,
                      conf_->fight_server_addr(),
                      ctx->zone->id(),
                      ctx->one->id(),
                      warrior_fight_result_callback);
    broker.SetOneCampWarriorRange(one_camp_choosen_warriors);
    broker.SetTheOtherCampWarriorRange(the_other_camp_choosen_warriors);
    broker.Wait();

    LOG_INFO << "Zone: " << ctx->zone->id() << ", one: " << ctx->one->id()
             << ", living warriors num: " << ctx->one->LivingWarriorsNum()
             << ", the other: " << ctx->the_other->id()
             << ", living warriors num: " << ctx->the_other->LivingWarriorsNum()
             << ", match: " << match;
  }

  ctx->winner = one_camp->NoLivingWarriors() ? the_other_camp : one_camp;
  DoWhenTwoCampsMatchDone(ctx);
  ctx->feeds_channel->WaitAllFeedsSended();
}

std::string ServerApp::CreateBackupKey(alpha::Slice key,
                                       const char* suffix,
                                       const char* server_id) {
  // Key: ##KEY##-##SERVERID##-##SUFFIX##
  std::ostringstream oss;
  oss << key.ToString() << '-';  // << server_id_ << '-' << suffix;
  if (server_id) {
    oss << server_id;
  } else {
    oss << server_id_;
  }
  oss << '-' << suffix;
  return oss.str();
}

void ServerApp::BackupRoutine(alpha::AsyncTcpClient* client,
                              alpha::AsyncTcpConnectionCoroutine* co,
                              bool check_last_backup_time) {
  if (is_backup_in_progress_) {
    LOG_INFO << "Last backup is still in progress";
    return;
  }
  if (check_last_backup_time) {
    auto now = alpha::Now();
    auto interval =
        std::max(now, last_backup_time_) - std::min(now, last_backup_time_);
    if (interval < conf_->backup_interval() * alpha::kMilliSecondsPerSecond) {
      LOG_INFO << "Giveup backup, last_backup_time_: "
               << alpha::to_time_t(last_backup_time_)
               << ", now: " << alpha::to_time_t(now)
               << ", interval: " << interval << "(ms), backup_interval: "
               << conf_->backup_interval() * alpha::kMilliSecondsPerSecond
               << "(ms)";
      return;
    }
  }
  is_backup_in_progress_ = true;
  auto reset_backup_status = [](bool* status) { *status = false; };
  std::unique_ptr<bool, decltype(reset_backup_status)> stub(
      &is_backup_in_progress_, reset_backup_status);

  LOG_INFO << "Starting backup, server id: " << server_id_;
  auto put = [](std::shared_ptr<alpha::AsyncTcpConnection> conn,
                alpha::Slice key,
                alpha::Slice val) {
    uint8_t kTTProtocolPutMagicFirst = 0xC8;
    uint8_t kTTProtocolPutMagicSecond = 0x10;
    uint32_t szkey = alpha::HostToBigEndian(static_cast<uint32_t>(key.size()));
    uint32_t szval = alpha::HostToBigEndian(static_cast<uint32_t>(val.size()));
    conn->Write(alpha::Slice(&kTTProtocolPutMagicFirst));
    conn->Write(alpha::Slice(&kTTProtocolPutMagicSecond));
    conn->Write(alpha::Slice(&szkey));
    conn->Write(alpha::Slice(&szval));
    conn->Write(key);
    conn->Write(val);
    LOG_INFO << "Try read response code";
    auto data = conn->Read(1);
    uint8_t rc = data[0];
    LOG_WARNING_IF(rc != 0) << "Failed response for put, rc: " << rc
                            << ", key: " << key.ToString();
    return rc;
  };
  const char* backup_suffix[] = {"tick", "tock"};
  static int backup_suffix_index = 0;
  const char* suffix = backup_suffix[backup_suffix_index];
  try {
    auto server_id = server_id_;
    auto conn = client->ConnectTo(conf_->backup_server_addr(), co);

    // 把瞬间状态保存到内存中
    alpha::IOBufferWithSize battle_data(battle_data_underlying_file_->size());
    memcpy(battle_data.data(),
           battle_data_underlying_file_->start(),
           battle_data.size());

    alpha::IOBufferWithSize warriors_data(
        warriors_data_underlying_file_->size());
    memcpy(warriors_data.data(),
           warriors_data_underlying_file_->start(),
           warriors_data.size());

    alpha::IOBufferWithSize rewards_data(rewards_data_underlying_file_->size());
    memcpy(rewards_data.data(),
           rewards_data_underlying_file_->start(),
           rewards_data.size());

    alpha::IOBufferWithSize rank_data(rank_data_underlying_file_->size());
    memcpy(rank_data.data(),
           rank_data_underlying_file_->start(),
           rank_data.size());

    auto key = CreateBackupKey(kBattleDataKey, suffix);
    auto err =
        put(conn, key, alpha::Slice(battle_data.data(), battle_data.size()));
    if (err) return;
    LOG_INFO << "Backup battle data done, key: " << key;

    key = CreateBackupKey(kWarriorsDataKey, suffix);
    err = put(
        conn, key, alpha::Slice(warriors_data.data(), warriors_data.size()));
    if (err) return;
    LOG_INFO << "Backup warriors data done, key: " << key;

    key = CreateBackupKey(kRewardsDataKey, suffix);
    err =
        put(conn, key, alpha::Slice(rewards_data.data(), rewards_data.size()));
    if (err) return;
    LOG_INFO << "Backup rewards data done, key: " << key;

    key = CreateBackupKey(kRankDataKey, suffix);
    err = put(conn, key, alpha::Slice(rank_data.data(), rank_data.size()));
    if (err) return;
    LOG_INFO << "Backup rank data done, key: " << key;

    LOG_INFO << "Backup done, server id: " << server_id
             << ", suffix: " << suffix;
    backup_suffix_index = 1 - backup_suffix_index;
    last_backup_time_ = alpha::Now();
    conn->Close();
  } catch (alpha::AsyncTcpConnectionException& e) {
    LOG_WARNING << "Backup failed, " << e.what();
  }
}

void ServerApp::RecoveryRoutine(alpha::AsyncTcpClient* client,
                                alpha::AsyncTcpConnectionCoroutine* co,
                                const char* server_id,
                                const char* suffix) {
  LOG_INFO << "Recovery started, server id: " << server_id
           << ", suffix: " << suffix;
  try {
    auto conn = client->ConnectTo(conf_->backup_server_addr(), co);
    auto ok = RecoverOneFile(conn.get(),
                             CreateBackupKey(kBattleDataKey, suffix, server_id),
                             battle_data_underlying_file_.get());
    if (!ok) return;

    ok = RecoverOneFile(conn.get(),
                        CreateBackupKey(kWarriorsDataKey, suffix, server_id),
                        warriors_data_underlying_file_.get());
    if (!ok) return;

    ok = RecoverOneFile(conn.get(),
                        CreateBackupKey(kRewardsDataKey, suffix, server_id),
                        rewards_data_underlying_file_.get());
    if (!ok) return;

    ok = RecoverOneFile(conn.get(),
                        CreateBackupKey(kRankDataKey, suffix, server_id),
                        rank_data_underlying_file_.get());
    if (!ok) return;

    LOG_INFO << "Recovery done, server id: " << server_id
             << ", suffix: " << suffix;
  } catch (alpha::AsyncTcpConnectionException& e) {
    LOG_ERROR << "Recovery failed, " << e.what();
  }
  loop_.Quit();
}

bool ServerApp::RecoverOneFile(alpha::AsyncTcpConnection* conn,
                               const std::string& backup_key,
                               alpha::MMapFile* file) {
  const uint8_t kTTProtocolGetMagicFirst = 0xC8;
  const uint8_t kTTProtocolGetMagicSecond = 0x30;
  try {
    LOG_INFO << "Start recover one file, backup key: " << backup_key;
    uint32_t szkey =
        alpha::HostToBigEndian(static_cast<uint32_t>(backup_key.size()));
    conn->Write(alpha::Slice(&kTTProtocolGetMagicFirst));
    conn->Write(alpha::Slice(&kTTProtocolGetMagicSecond));
    conn->Write(alpha::Slice(&szkey));
    conn->Write(backup_key);

    auto data = conn->Read(1);
    uint8_t rc = data[0];
    if (rc != 0) {
      LOG_ERROR << "Failed response, code: " << rc;
      return false;
    }

    data = conn->Read(sizeof(uint32_t));
    uint32_t szval =
        alpha::BigEndianToHost(*reinterpret_cast<const uint32_t*>(data.data()));
    LOG_INFO << "Backup value size: " << szval;

    if (szval > file->size()) {
      LOG_ERROR << "MMapFile size is too small, expect: " << szval
                << ", actual: " << file->size();
      return false;
    }
    data = conn->Read(szval);
    memcpy(file->start(), data.data(), szval);

    LOG_INFO << "Recovery done, backup key: " << backup_key
             << ", backup size: " << szval;
    return true;
  } catch (alpha::AsyncTcpConnectionException& e) {
    LOG_ERROR << "RecoverOneFile catch AsyncTcpConnectionException, "
              << e.what();
    return false;
  }
}

void ServerApp::ProcessFightTaskResult(
    BattleContext* ctx, const FightServerProtocol::TaskResult& result) {
  for (const auto& fight_result : result.fight_pair_result()) {
    auto challenger = fight_result.challenger();
    auto defender = fight_result.defender();
    if (fight_result.error() == 0) {
      UinType winner = fight_result.winner();
      UinType loser;
      const std::string* loser_view_fight_content = nullptr;
      if (winner == challenger) {
        loser = defender;
        loser_view_fight_content = &fight_result.defender_view_fight_content();
      } else {
        loser = challenger;
        loser_view_fight_content =
            &fight_result.challenger_view_fight_content();
      }
      ProcessSurvivedWarrior(ctx, winner, loser);
      ProcessDeadWarrior(ctx, loser, winner, *loser_view_fight_content);
    } else {
      LOG_WARNING << "Fight failed, challenger: " << challenger
                  << ", defender: " << defender
                  << ", error: " << fight_result.error();
    }
  }
}

void ServerApp::ProcessSurvivedWarrior(BattleContext* ctx,
                                       UinType winner,
                                       UinType loser) {
  auto it = warriors_->find(winner);
  if (it == warriors_->end()) {
    LOG_WARNING << "Cannot find warrior, winner: " << winner;
  }
  auto& warrior = it->second;
  auto camp = ctx->zone->GetCamp(warrior.camp_id());
  // 更新击杀数
  warrior.add_killing_num();
  warrior.set_last_killed_warrior(loser);
  ReportKillingNumToRank(ctx->zone->id(), winner, warrior.season_killing_num());
  ctx->zone->leaders()->Notify(
      camp->id(), winner, warrior.season_killing_num());
}

void ServerApp::ProcessRoundSurvivedWarrior(BattleContext* ctx, UinType uin) {
  auto it = warriors_->find(uin);
  // 从Camp中取出的uin, 直接CHECK
  CHECK(it != warriors_->end()) << "Cannot find warrior, uin: " << uin;
  auto& warrior = it->second;
  bool bye = warrior.last_killed_warrior() == 0;
  if (bye) {
    FeedsServerProtocol::ThronesBattleBye feeds;
    feeds.set_src(uin);
    feeds.set_dst(uin);
    feeds.set_season(battle_data_->CurrentSeason());
    feeds.set_zone(ctx->zone->id());
    feeds.set_round(ctx->zone->matchups()->CurrentRound());
    ctx->feeds_channel->AddFightMessage(kThronesBattleBye, &feeds);
  } else {
    // 写本轮胜利feeds
    FeedsServerProtocol::ThronesBattleWin feeds;
    feeds.set_src(uin);
    feeds.set_dst(uin);
    feeds.set_season(battle_data_->CurrentSeason());
    feeds.set_zone(ctx->zone->id());
    feeds.set_round(ctx->zone->matchups()->CurrentRound());
    feeds.set_round_killing_num(warrior.round_killing_num());
    feeds.set_last_killed_warrior(warrior.last_killed_warrior());
    ctx->feeds_channel->AddFightMessage(kThronesBattleWin, &feeds);
  }
}

void ServerApp::DoWhenSeasonChanged() {
  LOG_INFO << "Season changed, new season: " << battle_data_->CurrentSeason();
  // 保存上一届的参赛人员信息, 目前用于换届后领奖
  for (const auto& p : *warriors_) {
    auto warrior_lite =
        WarriorLite::Create(p.first, p.second.zone_id(), p.second.camp_id());
    rewards_->insert(alpha::make_pod_pair(p.first, warrior_lite));
  }
  // 换届干掉上一届的参战人员
  warriors_->clear();
  AddTimerForBattleRound();
}

void ServerApp::DoWhenSeasonFinished() {
  LOG_INFO << "Season " << battle_data_->CurrentSeason() << " finished";
  battle_data_->SetSeasonFinished();
  battle_data_->ResetLuckyWarriors();
  AddTimerForChangeSeason();
}

void ServerApp::DoWhenRoundFinished() {
  CHECK(battle_data_->CurrentRoundFinished());
  auto current = battle_data_->CurrentRound();
  LOG_INFO << "Season: " << battle_data_->CurrentSeason() << ", Round "
           << current << " finished";
  if (current == kMaxRoundID) {
    DoWhenSeasonFinished();
  } else {
    AddTimerForBattleRound();
  }
}

void ServerApp::DoWhenTwoCampsMatchDone(BattleContext* ctx) {
  CHECK(ctx->winner);
  CHECK(ctx->winner == ctx->one || ctx->winner == ctx->the_other);
  LOG_INFO << "Battle done, Zone: " << ctx->zone->id()
           << " one: " << ctx->one->id()
           << ", the_other: " << ctx->the_other->id()
           << ", winner camp: " << ctx->winner->id();

  // 更新对战结果
  ctx->zone->matchups()->SetBattleResult(
      ctx->one->id(), ctx->one == ctx->winner, ctx->one->LivingWarriorsNum());
  ctx->zone->matchups()->SetBattleResult(ctx->the_other->id(),
                                         ctx->the_other == ctx->winner,
                                         ctx->the_other->LivingWarriorsNum());

  // 判断当前所有赛区本轮所有比赛是否已经打完
  // 记录瞬间状态，否则在写完每轮Feeds后可能多个协程同时进入每轮结束流程
  bool round_finished = battle_data_->CurrentRoundFinished();

  // 为获胜阵营所有仍然存活的人写feeds
  auto& living_warriors = ctx->winner->living_warriors();
  for (auto uin : living_warriors) {
    ProcessRoundSurvivedWarrior(ctx, uin);
  }

  WriteRankFeedsIfSeasonFinished(ctx, ctx->one);
  WriteRankFeedsIfSeasonFinished(ctx, ctx->the_other);

  if (round_finished) {
    DoWhenRoundFinished();
  }
}

void ServerApp::WriteRankFeedsIfSeasonFinished(BattleContext* ctx, Camp* camp) {
  CHECK(camp == ctx->one || camp == ctx->the_other);
  auto rank = ctx->zone->matchups()->FinalRank(camp->id());
  if (rank) {
    CHECK(rank <= kCampIDMax);
    LOG_INFO << "Season: " << battle_data_->CurrentSeason()
             << ", zone: " << ctx->zone->id() << ", camp: " << camp->id()
             << ", rank: " << rank;
    for (const auto& uin : camp->Warriors()) {
      FeedsServerProtocol::ThronesBattleCampRank feeds;
      feeds.set_src(uin);
      feeds.set_dst(uin);
      feeds.set_season(battle_data_->CurrentSeason());
      feeds.set_zone(ctx->zone->id());
      feeds.set_rank(rank);
      ctx->feeds_channel->AddFightMessage(kThronesBattleCampRank[rank - 1],
                                          &feeds);
    }

    // 冠军阵营领军人记为大将军
    if (rank == 1) {
      ctx->zone->generals()->AddGeneralInChief(
          camp->id(),
          ctx->zone->leaders()->GetLeader(camp->id()).uin,
          battle_data_->CurrentSeason());
    }
  }
}

void ServerApp::ReportKillingNumToRank(uint16_t zone_id,
                                       UinType uin,
                                       uint32_t killing_num) {
  auto it = season_ranks_.find(zone_id);
  CHECK(it != season_ranks_.end());
  it->second->Report(uin, killing_num);
}

void ServerApp::ProcessDeadWarrior(BattleContext* ctx,
                                   UinType loser,
                                   UinType winner,
                                   const std::string& fight_content) {
  auto it = warriors_->find(loser);
  CHECK(it != warriors_->end()) << "Cannot find warrior, loser: " << loser;
  auto& warrior = it->second;
  auto camp = ctx->zone->GetCamp(warrior.camp_id());
  warrior.set_dead(true);
  camp->MarkWarriorDead(loser);
  FeedsServerProtocol::ThronesBattleLose feeds;
  feeds.set_src(loser);
  feeds.set_dst(winner);
  feeds.set_season(battle_data_->CurrentSeason());
  feeds.set_zone(ctx->zone->id());
  feeds.set_round(ctx->zone->matchups()->CurrentRound());
  feeds.set_round_killing_num(warrior.round_killing_num());
  feeds.set_fight_content(fight_content);
  ctx->feeds_channel->AddFightMessage(kThronesBattleLose, &feeds);
}

void ServerApp::AddTimerForChangeSeason() {
  CHECK(battle_data_->SeasonFinished());
  alpha::TimeStamp at = alpha::from_time_t(conf_->NextSeasonBaseTime());
  LOG_INFO << "Will change season at " << at;
  loop_.RunAt(at, [this] {
    bool done = battle_data_->ChangeSeason();
    if (done) {
      DoWhenSeasonChanged();
    } else {
      LOG_WARNING
          << "Change season failed, maybe multiple timers for season change";
    }
  });
}

void ServerApp::AddTimerForBattleRound() {
  CHECK(!battle_data_->SeasonFinished());
  auto finished_round = battle_data_->FinishedRound();
  CHECK(finished_round < kMaxRoundID);
  auto next_round = finished_round + 1;
  alpha::TimeStamp at =
      alpha::from_time_t(conf_->BattleRoundStartTime(next_round));
  LOG_INFO << "Will start round " << next_round << " at " << at;
  loop_.RunAt(at, [this] { RunBattle(); });
}

void ServerApp::AddTimerForBackup() {
  // 防止某次备份失败后要等下一个Interval才能继续备份
  auto check_interval =
      conf_->backup_interval() / 10 * alpha::kMilliSecondsPerSecond;
  static const int kMinCheckBackupInterval = 1000;
  if (check_interval < kMinCheckBackupInterval) {
    check_interval = kMinCheckBackupInterval;
  }
  DLOG_INFO << "Check backup interval: " << check_interval;
  loop_.RunEvery(check_interval, [this] {
    async_tcp_client_.RunInCoroutine(
        std::bind(&ServerApp::BackupRoutine, this, _1, _2, true));
  });
}

void ServerApp::InitBeforeNewSeasonBattle() {
  CHECK(battle_data_->SeasonNotStarted());
  DLOG_INFO << "Reset season data";
  // 新赛季开始，清空旧赛季的奖励名单
  rewards_->clear();
  battle_data_->ResetLeaders();
  battle_data_->ResetMatchups();
  // 清空旧赛季的排行榜
  for (auto& p : season_ranks_) {
    p.second->Clear();
  }
}

void ServerApp::RunBattle() {
  if (battle_data_->SeasonNotStarted()) {
    InitBeforeNewSeasonBattle();
    battle_data_->IncreaseCurrentRound();
  }
  CHECK(!battle_data_->SeasonFinished());
  StartAllZonesToCurrentRound();

  auto unfinished_battle_tasks = GetAllUnfinishedTasks();
  LOG_INFO << "Season: " << battle_data_->CurrentSeason()
           << ", Round: " << battle_data_->CurrentRound()
           << ", Unfinished task: " << unfinished_battle_tasks.size();
  if (!unfinished_battle_tasks.empty()) {
    for (auto& task : unfinished_battle_tasks) {
      RunBattleTask(task);
    }
  } else if (battle_data_->CurrentRound() == kMaxRoundID) {
    // 已经打完了
    DoWhenSeasonFinished();
  } else {
    // 准备下一轮
    battle_data_->IncreaseCurrentRound();
    AddTimerForBattleRound();
  }
}

std::vector<BattleTask> ServerApp::GetAllUnfinishedTasks() {
  std::vector<BattleTask> tasks;
  auto current_round = battle_data_->CurrentRound();
  battle_data_->ForeachZone([&tasks, current_round](Zone& zone) {
    auto fill = [&tasks, &zone](const MatchupData* one,
                                const MatchupData* the_other) {
      BattleTask task;
      task.zone = &zone;
      task.one = one->camp;
      task.the_other = the_other->camp;
      tasks.push_back(task);
    };
    zone.matchups()->UnfinishedBattle(current_round, fill);
  });
  return tasks;
}

void ServerApp::RunBattleTask(BattleTask task) {
  async_tcp_client_.RunInCoroutine(std::bind(&ServerApp::RoundBattleRoutine,
                                             this,
                                             _1,
                                             _2,
                                             task.zone,
                                             task.one,
                                             task.the_other));
}

void ServerApp::StartAllZonesToCurrentRound() {
  const unsigned current_round = battle_data_->CurrentRound();
  for (uint16_t zone_id = kMinZoneID; zone_id <= kMaxZoneID; ++zone_id) {
    auto zone = battle_data_->GetZone(zone_id);
    auto zone_current_round = zone->matchups()->CurrentRound();
    if (zone_current_round == current_round) {
      // 已经开始本轮
    } else if (zone_current_round == current_round - 1) {
      // 开启本轮
      bool ok = zone->matchups()->StartNextRound();
      CHECK(ok) << "Start next round failed, zone_id: " << zone_id;
      LOG_INFO << "Set zone(" << zone_id << ") to round("
               << zone->matchups()->CurrentRound() << ")";

      //重置所有赛场玩家的存活状态
      for (uint16_t i = 0; i < kCampIDMax; ++i) {
        auto camp = zone->GetCamp(i + 1);
        camp->ResetLivingWarriors();
        camp->ForeachWarrior([this](UinType uin) {
          auto it = warriors_->find(uin);
          CHECK(it != warriors_->end());
          it->second.ResetRoundData();
        });
      }
    } else {
      CHECK(false) << "Invalid zone current round: " << zone_current_round
                   << ", current round: " << current_round;
    }
  }
}

int ServerApp::Run() {
  if (conf_->daemonize()) {
    /* Daemonize */
    int err = daemon(0, 0);
    if (err) {
      PLOG_ERROR << "daemon";
      return err;
    }
  }
  if (!CreatePidFile()) {
    LOG_ERROR << "Create PidFile failed";
    return EXIT_FAILURE;
  }
  /* Trap signals */
  TrapSignals();
  udp_server_.SetMessageCallback(
      std::bind(&ServerApp::HandleUDPMessage, this, _1, _2, _3, _4));
  bool ok = udp_server_.Run(conf_->service_addr());
  if (!ok) return EXIT_FAILURE;
  ok = http_server_.Run(conf_->admin_addr());
  if (!ok) return EXIT_FAILURE;
  loop_.Run();
  return EXIT_SUCCESS;
}

void ServerApp::HandleUDPMessage(alpha::UDPSocket* socket,
                                 alpha::IOBuffer* buf,
                                 size_t buf_len,
                                 const alpha::NetAddress& peer) {
  DLOG_INFO << "Receive UDP message, size: " << buf_len;
  ThronesBattleServerProtocol::RequestWrapper request_wrapper;
  bool ok = request_wrapper.ParseFromArray(buf->data(), buf_len);
  if (unlikely(!ok)) {
    LOG_WARNING << "Cannot parse RequestWrapper from message\n"
                << alpha::HexDump(alpha::Slice(buf->data(), buf_len));
    return;
  }
  DLOG_INFO << "Receive request, ctx: " << request_wrapper.ctx()
            << ", uin: " << request_wrapper.uin()
            << ", payload name: " << request_wrapper.payload_name()
            << ", payload size: " << request_wrapper.payload().size();
  using namespace google::protobuf;
  auto descriptor = DescriptorPool::generated_pool()->FindMessageTypeByName(
      request_wrapper.payload_name());
  if (unlikely(descriptor == nullptr)) {
    LOG_WARNING << "Cannot find message type, name: "
                << request_wrapper.payload_name();
    return;
  }
  auto prototype =
      MessageFactory::generated_factory()->GetPrototype(descriptor);
  if (unlikely(prototype == nullptr)) {
    LOG_WARNING << "Cannot get prototype for message, name: "
                << request_wrapper.payload_name();
    return;
  }
  std::unique_ptr<Message> message(prototype->New());
  ok = message->ParseFromString(request_wrapper.payload());
  if (unlikely(!ok)) {
    LOG_WARNING << "Cannot parse from payload\n"
                << alpha::HexDump(request_wrapper.payload());
    return;
  }
  DLOG_INFO << "Request DebugString:\n" << message->DebugString();

  ResponseWrapper response_wrapper;
  response_wrapper.set_ctx(request_wrapper.ctx());
  response_wrapper.set_uin(request_wrapper.uin());
  auto rc = message_dispatcher_.Dispatch(
      request_wrapper.uin(), message.get(), &response_wrapper);
  response_wrapper.set_rc(rc);
  DLOG_INFO << "Dispatch returns " << rc;
  alpha::IOBufferWithSize out(response_wrapper.ByteSize());
  ok = response_wrapper.SerializeToArray(out.data(), out.size());
  if (unlikely(!ok)) {
    LOG_ERROR << "ResponseWrapper::SerializeToArray failed";
    return;
  }
  rc = socket->SendTo(&out, out.size(), peer);
  if (rc < 0) {
    PLOG_WARNING << "UDPSocket::SendTo failed";
  } else {
    DLOG_INFO << "Send udp message to " << peer << ", size: " << out.size();
  }
}

int ServerApp::HandleQuerySignUp(UinType uin,
                                 const QuerySignUpRequest* req,
                                 QuerySignUpResponse* resp) {
  if (unlikely(uin == 0)) {
    return Error::kInvalidArgument;
  }
  resp->set_season(battle_data_->CurrentSeason());
  auto it = warriors_->find(uin);
  if (it != warriors_->end()) {
    resp->set_zone(it->second.zone_id());
    resp->set_camp(it->second.camp_id());
  }
  return Error::kOk;
}

int ServerApp::HandleSignUp(UinType uin,
                            const SignUpRequest* req,
                            SignUpResponse* resp) {
  if (unlikely(uin == 0)) {
    return Error::kInvalidArgument;
  }
  if (unlikely(warriors_->size() == warriors_->max_size())) {
    return Error::kNoSpaceForWarrior;
  }
  if (!conf_->InSignUpTime()) {
    return Error::kNotInSignUpTime;
  }
  if (warriors_->find(uin) != warriors_->end()) {
    return Error::kAlreadySignedUp;
  }
  std::vector<Zone*> zones;
  battle_data_->ForeachZone([&zones](Zone& zone) { zones.push_back(&zone); });
  std::sort(
      zones.begin(), zones.end(), [this](const Zone* lhs, const Zone* rhs) {
        auto lhs_conf = conf_->GetZoneConf(lhs);
        auto rhs_conf = conf_->GetZoneConf(rhs);
        return lhs_conf->max_level() < rhs_conf->max_level();
      });
  auto user_level = req->level();
  auto it = std::find_if(
      zones.begin(), zones.end(), [this, user_level](const Zone* zone) {
        auto zone_conf = conf_->GetZoneConf(zone);
        return user_level <= zone_conf->max_level();
      });
  if (it == zones.end()) {
    return Error::kServerInternalError;
  }
  Zone* zone = nullptr;
  Camp* camp = nullptr;
  while (it != zones.end()) {
    // 从低等级到高等级遍历赛区
    zone = *it;
    auto zone_conf = conf_->GetZoneConf(zone);
    std::vector<Camp*> candidate_camps;
    for (auto i = 0u; i < kCampIDMax; ++i) {
      auto camp = zone->GetCamp(i + 1);
      if (camp->WarriorsNum() < zone_conf->max_camp_warriors_num()) {
        candidate_camps.push_back(camp);
      }
    }
    if (candidate_camps.empty()) {
      ++it;
      continue;
    }
    auto camps = alpha::Random::Sample(
        candidate_camps.begin(), candidate_camps.end(), 1);
    CHECK(camps.size() == 1u);
    camp = *camps.front();
    break;
  }
  if (camp == nullptr) {
    return Error::kAllZoneAreFull;
  }
  auto zone_conf = conf_->GetZoneConf(zone);
  CHECK(user_level <= zone_conf->max_level())
      << "Invalid zone found , user level: " << user_level
      << ", zone max level: " << zone_conf->max_level();
  auto p = warriors_->insert(
      alpha::make_pod_pair(uin, Warrior::Create(uin, zone->id(), camp->id())));
  CHECK(p.second) << "Insert new warrior failed";
  camp->AddWarrior(uin);
  resp->set_season(battle_data_->CurrentSeason());
  resp->set_zone(zone->id());
  resp->set_camp(camp->id());
  return Error::kOk;
}

int ServerApp::HandleQueryBattleStatus(UinType uin,
                                       const QueryBattleStatusRequest* req,
                                       QueryBattleStatusResponse* resp) {
  if (unlikely(uin == 0)) {
    return Error::kInvalidArgument;
  }
  bool has_zone = req->has_zone();
  bool has_round = req->has_round();
  uint16_t zone_id = req->zone();
  uint16_t battle_round = req->round();
  if (unlikely(has_zone && !ValidZoneID(zone_id))) {
    return Error::kInvalidArgument;
  }
  if (unlikely(has_round &&
               (battle_round == 0 || battle_round > kMaxRoundID))) {
    return Error::kInvalidArgument;
  }

  if (!has_zone) {
    //规则:
    // 1.已报名则显示报名赛区
    // 2.未报名则显示无限制赛区
    auto it = warriors_->find(uin);
    zone_id = it == warriors_->end() ? kUnlimitedZoneID : it->second.zone_id();
  }
  CHECK(ValidZoneID(zone_id)) << "Invalid zone id: " << zone_id;

  if (!has_round) {
    //规则:
    // 1.本届还没开打显示上届第三轮
    // 2.本届打完显示本届第三轮
    // 3.当前正在打某一轮，则显示该轮，否则显示最近一轮
    if (battle_data_->SeasonNotStarted() || battle_data_->SeasonFinished()) {
      battle_round = kMaxRoundID;
    } else {
      battle_round = battle_data_->CurrentRound();
    }
  }

  CHECK(battle_round != 0 && battle_round <= kMaxRoundID)
      << "Invalid battle round: " << battle_round;

  resp->set_season(battle_data_->CurrentSeason());
  auto it = warriors_->find(uin);
  if (it != warriors_->end()) {
    resp->set_signed_up_zone(it->second.zone_id());
    resp->set_signed_up_camp(it->second.camp_id());
  }
  auto zone = battle_data_->GetZone(zone_id);
  for (auto i = 0u; i < kCampIDMax; ++i) {
    auto camp_proto = resp->add_camp();
    auto camp = zone->GetCamp(i + 1);
    // 如果本届已经开战，那么查询的轮应该小于等于当前轮
    if (battle_data_->SeasonStarted() &&
        battle_round > battle_data_->CurrentRound()) {
      return Error::kInvalidArgument;
    }
    camp_proto->set_id(camp->id());
    camp_proto->set_warriors(camp->WarriorsNum());
    // 规则:
    //  1.查看当前正在打的这轮，从camps中提取实时剩余人数
    //  2.查看已经打完的某一轮，从matchups里面提取历史记录
    if (battle_data_->InitialSeason() || (battle_data_->CurrentSeason() == 1 &&
                                          battle_data_->SeasonNotStarted())) {
      camp_proto->set_living_warriors(0);
    } else if (!battle_data_->CurrentRoundFinished() &&
               battle_round == battle_data_->CurrentRound()) {
      // 查看本届未完成的某轮信息
      camp_proto->set_living_warriors(camp->LivingWarriorsNum());
    } else {
      // 查看上届/本届已完成某轮的对阵信息
      camp_proto->set_living_warriors(
          zone->matchups()->GetFinalLivingWarriorsNum(battle_round,
                                                      camp->id()));
    }
    auto leader = zone->leaders()->GetLeader(camp->id());
    if (!leader.empty()) {
      camp_proto->set_leader(leader.uin);
      camp_proto->set_leader_killing_num(leader.killing_num);
      // 赛季完结后才能挑选福将
      camp_proto->set_can_pick_lucky_warriors(battle_data_->SeasonFinished() &&
                                              !leader.picked_lucky_warriors);
    }
    if (zone->lucky_warriors()->PickedLuckyWarriors(camp->id())) {
      auto lucky_warriors = zone->lucky_warriors()->Get(camp->id());
      auto lucky_warriors_proto = resp->add_lucky_warriors();
      lucky_warriors_proto->set_camp(camp->id());
      std::copy(std::begin(lucky_warriors),
                std::end(lucky_warriors),
                google::protobuf::RepeatedFieldBackInserter(
                    lucky_warriors_proto->mutable_lucky_warrior()));
    }
  }
  auto matchups_proto = resp->mutable_matchups();
  if (battle_data_->InitialSeason() || (battle_data_->CurrentSeason() == 1 &&
                                        battle_data_->SeasonNotStarted())) {
    // 初始赛季没有对阵列表，来个pesudo matchups
    for (uint16_t i = 0; i < kCampIDMax; i += 2) {
      auto one = i + 1;
      auto the_other = i + 2;
      auto round_matchups = matchups_proto->add_round_matchups();
      round_matchups->set_one_camp(one);
      round_matchups->set_the_other_camp(the_other);
      round_matchups->set_winner_camp(0);
    }
  } else {
    auto matchups = zone->matchups();
    if (battle_round > matchups->CurrentRound()) {
      return Error::kRoundNotStarted;
    }
    matchups_proto->set_zone(zone->id());
    auto cb = [matchups_proto](const MatchupData* one,
                               const MatchupData* the_other) {
      auto round_matchups = matchups_proto->add_round_matchups();
      round_matchups->set_one_camp(one->camp);
      round_matchups->set_the_other_camp(the_other->camp);
      if (one->win) {
        round_matchups->set_winner_camp(one->camp);
      } else if (the_other->win) {
        round_matchups->set_winner_camp(the_other->camp);
      } else {
        round_matchups->set_winner_camp(0);
      }
    };
    matchups->ForeachMatchup(battle_round, cb);
  }
  auto current_season = battle_data_->CurrentSeason();
  CHECK(current_season >= 1);
  if (battle_data_->SeasonFinished()) {
    resp->set_lucky_warriors_season(current_season);
  } else if (current_season == 1) {
    resp->set_lucky_warriors_season(1);  // 第一届还没打完，不能显示第0届啊
  } else {
    resp->set_lucky_warriors_season(current_season - 1);
  }
  bool is_lucky_warrior = false;
  battle_data_->ForeachZone([uin, &is_lucky_warrior](const Zone& zone) {
    is_lucky_warrior =
        is_lucky_warrior || zone.lucky_warriors()->HasWarrior(uin);
  });
  resp->set_is_lucky_warrior(is_lucky_warrior);
  resp->set_show_zone(zone_id);
  resp->set_show_round(battle_round);
  resp->set_in_sign_up_time(conf_->InSignUpTime());
  bool show_round_finished = true;
  if (battle_round == battle_data_->CurrentRound() &&
      !battle_data_->CurrentRoundFinished()) {
    show_round_finished = false;
  }
  resp->set_show_round_finished(show_round_finished);
  if (battle_data_->SeasonStarted()) {
    resp->set_max_show_round(battle_data_->CurrentRound());
  } else {
    resp->set_max_show_round(kMaxRoundID);
  }
  if (!battle_data_->InitialSeason() &&
      (battle_data_->SeasonNotStarted() || battle_data_->SeasonFinished())) {
    for (int i = 0; i < kCampIDMax; ++i) {
      CampID camp_id = (CampID)(i + 1);
      auto rank = zone->matchups()->FinalRank(camp_id);
      if (rank == 1) {
        resp->set_current_season_champion_camp(camp_id);
        break;
      }
    }
  }
  return Error::kOk;
}

int ServerApp::HandlePickLuckyWarriors(UinType uin,
                                       const PickLuckyWarriorsRequest* req,
                                       PickLuckyWarriorsResponse* resp) {
  if (unlikely(uin == 0)) {
    return Error::kInvalidArgument;
  }
  if (!battle_data_->SeasonFinished()) {
    // 本届打完之后，才能挑选福将
    return Error::kNotInPickLuckyWarriorsTime;
  }
  auto it = warriors_->find(uin);
  if (it == warriors_->end()) {
    return Error::kNotSignedUp;
  }

  auto zone = battle_data_->GetZone(it->second.zone_id());
  auto camp_id = it->second.camp_id();
  auto camp_leader = zone->leaders()->GetLeader(camp_id);
  if (camp_leader.uin != uin) {
    DLOG_INFO << "uin: " << uin << ", leader: " << camp_leader.uin;
    return Error::kNotCampLeader;
  }
  if (camp_leader.picked_lucky_warriors) {
    return Error::kPickedLuckyWarriros;
  }
  auto camp = zone->GetCamp(camp_id);
  auto begin = camp->Warriors().begin();
  auto end = camp->Warriors().end();
  auto lucky_warriors_iters =
      alpha::Random::Sample(begin, end, kMaxLuckyWarroirPerCamp);
  resp->set_zone(zone->id());
  resp->mutable_lucky_warriors()->set_camp(camp_id);
  for (const auto& it : lucky_warriors_iters) {
    UinType uin = *it;
    zone->lucky_warriors()->AddLuckyWarrior(camp_id, uin);
    resp->mutable_lucky_warriors()->add_lucky_warrior(uin);
  }
  zone->leaders()->SetPickedLuckyWarriors(camp_id);
  return Error::kOk;
}

int ServerApp::HandleQueryLuckyWarriorReward(
    UinType uin,
    const QueryLuckyWarriorRewardRequest* req,
    QueryLuckyWarriorRewardResponse* resp) {
  if (unlikely(uin == 0)) {
    return Error::kInvalidArgument;
  }
  if (!conf_->InRewardTime(battle_data_->SeasonFinished())) {
    return Error::kNotInRewardTime;
  }
  // TODO: 太他妈丑了
  bool is_lucky_warrior = false;
  battle_data_->ForeachZone([uin, &is_lucky_warrior](const Zone& zone) {
    if (is_lucky_warrior) return;
    if (zone.lucky_warriors()->HasWarrior(uin)) {
      is_lucky_warrior = true;
    }
  });
  if (!is_lucky_warrior) {
    return Error::kNotLuckyWarrior;
  }
  unsigned reward_season = battle_data_->SeasonFinished()
                               ? battle_data_->CurrentSeason()
                               : battle_data_->CurrentSeason() - 1;
  resp->set_season(reward_season);
  return Error::kOk;
}

int ServerApp::HandleQueryRoundRewardRequest(UinType uin,
                                             const QueryRoundRewardRequest* req,
                                             QueryRoundRewardResponse* resp) {
  if (unlikely(uin == 0)) {
    return Error::kInvalidArgument;
  }
  // 初始赛季
  if (battle_data_->InitialSeason()) {
    return Error::kNoRoundReward;
  }
  // 第一届还没开打时
  if (battle_data_->CurrentSeason() == 1 && battle_data_->SeasonNotStarted()) {
    return Error::kNoRoundReward;
  }
  // 第一轮开打了,但是还没打完
  if (battle_data_->SeasonStarted() && battle_data_->CurrentRound() == 1 &&
      !battle_data_->CurrentRoundFinished()) {
    return Error::kNoRoundReward;
  }

  uint16_t reward_season = 0;
  uint16_t zone_id = 0;
  uint16_t camp_id = 0;
  if (battle_data_->SeasonNotStarted()) {
    // 本届没开打，领取上届奖励
    auto it = rewards_->find(uin);
    if (it == rewards_->end()) {
      // 没有参加上届比赛
      return Error::kNoLastSeasonWarrior;
    }
    reward_season = battle_data_->CurrentSeason() - 1;
    zone_id = it->second.zone_id;
    camp_id = it->second.camp_id;
  } else {
    CHECK(battle_data_->FinishedRound() >= 1);
    auto it = warriors_->find(uin);
    if (it == warriors_->end()) {
      // 没有参加本届比赛
      return Error::kNotSignedUp;
    }
    reward_season = battle_data_->CurrentSeason();
    zone_id = it->second.zone_id();
    camp_id = it->second.camp_id();
  }

  CHECK(reward_season && zone_id && camp_id);
  CHECK(ValidCampID(camp_id));
  resp->set_season(reward_season);
  resp->set_zone(zone_id);
  resp->set_camp(camp_id);
  auto zone = battle_data_->GetZone(zone_id);
  auto zone_finished_round = zone->matchups()->FinishedRound();
  auto game_log =
      zone->matchups()->GetGameLog((CampID)camp_id, zone_finished_round);
  resp->set_game_log(game_log);
  if (!battle_data_->InitialSeason()) {
    bool found = false;
    auto general = zone->generals()->GetBySeason(reward_season, &found);
    if (found) {
      resp->set_zone_general(general.uin);
    }
  }
  return Error::kOk;
}

int ServerApp::HandleQueryGeneralInChief(UinType uin,
                                         const QueryGeneralInChiefRequest* req,
                                         QueryGeneralInChiefResponse* resp) {
  auto zone_id = req->zone();
  if (!ValidZoneID(zone_id)) {
    return Error::kInvalidArgument;
  }
  auto zone = battle_data_->GetZone(zone_id);
  auto generals = zone->generals()->Get(req->start(), req->num());
  DLOG_INFO << "generals.size() =  " << generals.size();
  resp->set_total(zone->generals()->Size());
  std::transform(
      generals.begin(),
      generals.end(),
      google::protobuf::AllocatedRepeatedPtrFieldBackInserter(
          resp->mutable_general()),
      [](const ThronesBattle::General& general) {
        auto general_proto =
            alpha::make_unique<ThronesBattleServerProtocol::General>();
        general_proto->set_uin(general.uin);
        general_proto->set_camp(general.camp);
        general_proto->set_season(general.season);
        return general_proto.release();
      });
  return Error::kOk;
}

int ServerApp::HandleQueryRank(UinType uin,
                               const QueryRankRequest* req,
                               QueryRankResponse* resp) {
  if (unlikely(uin == 0)) {
    return Error::kInvalidArgument;
  }
  uint16_t zone_id = 0;
  if (req->has_zone()) {
    zone_id = req->zone();
    if (!ValidZoneID(zone_id)) {
      return Error::kInvalidArgument;
    }
  } else {
    if (battle_data_->SeasonNotStarted()) {
      auto it = rewards_->find(uin);
      zone_id = it == rewards_->end() ? kUnlimitedZoneID : it->second.zone_id;
    } else {
      auto it = warriors_->find(uin);
      zone_id =
          it == warriors_->end() ? kUnlimitedZoneID : it->second.zone_id();
    }
  }
  CHECK(ValidZoneID(zone_id));
  auto type = req->type();
  auto ranks = &history_ranks_;
  if (type == 1) {
    // 本届
    unsigned rank_season = 0;
    bool rank_season_finished = false;
    ranks = &season_ranks_;
    if (battle_data_->SeasonFinished()) {
      // 本届已结束
      rank_season = battle_data_->CurrentSeason();
      rank_season_finished = true;
    } else if (battle_data_->SeasonNotStarted()) {
      // 本届还没开始，这是上一届的排行榜
      rank_season = battle_data_->CurrentSeason() - 1;
      rank_season_finished = true;
    } else {
      // 本届已开战，还没打完
      rank_season = battle_data_->CurrentSeason();
      rank_season_finished = false;
    }
    resp->set_rank_season(rank_season);
    resp->set_rank_season_finished(rank_season_finished);
    resp->set_zone(zone_id);
  }
  auto it = ranks->find(zone_id);
  CHECK(it != ranks->end());
  auto& rank = it->second;
  resp->set_self_rank(rank->Rank(uin));
  resp->set_total(rank->Size());
  auto v = rank->GetRange(req->start(), req->num());
  std::transform(v.begin(),
                 v.end(),
                 google::protobuf::AllocatedRepeatedPtrFieldBackInserter(
                     resp->mutable_warriors()),
                 [](const RankVectorUnit& u) {
                   auto rank_unit = alpha::make_unique<RankUnit>();
                   rank_unit->set_uin(u.uin);
                   rank_unit->set_val(u.val);
                   return rank_unit.release();
                 });
  return Error::kOk;
}

int ServerApp::HandleQueryWarriorRank(UinType uin,
                                      const QueryWarriorRankRequest* req,
                                      QueryWarriorRankResponse* resp) {
  if (unlikely(uin == 0)) {
    return Error::kInvalidArgument;
  }
  if (battle_data_->SeasonNotStarted()) {
    auto it = rewards_->find(uin);
    if (it != rewards_->end()) {
      resp->set_zone(it->second.zone_id);
      resp->set_camp(it->second.camp_id);
      auto& rank = *season_ranks_[it->second.zone_id];
      resp->set_rank(rank.Rank(uin));
    }
  } else {
    auto it = warriors_->find(uin);
    if (it != warriors_->end()) {
      resp->set_zone(it->second.zone_id());
      resp->set_camp(it->second.camp_id());
      auto& rank = *season_ranks_[it->second.zone_id()];
      resp->set_rank(rank.Rank(uin));
    }
  }
  return Error::kOk;
}
}
