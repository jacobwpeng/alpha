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
#include <alpha/format.h>
#include <alpha/logger.h>
#include <alpha/random.h>
#include "ThronesBattleSvrdConf.h"
#include "ThronesBattleSvrdTaskBroker.h"
#include "ThronesBattleSvrd.pb.h"
#include "ThronesBattleSvrdFeedsUtil.h"
#include "fightsvrd.pb.h"

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
ServerApp::ServerApp() : async_tcp_client_(&loop_), udp_server_(&loop_) {}

ServerApp::~ServerApp() = default;

int ServerApp::Init(const char* file) {
  conf_ = ServerConf::Create(file);
  if (conf_ == nullptr) {
    return EXIT_FAILURE;
  }

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

  return RecoveryMode() ? InitRecoveryMode() : InitNormalMode();
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

  auto err = feeds_socket_.Open();
  if (err) {
    PLOG_ERROR << "Open feeds socket failed, err: " << err;
    return EXIT_FAILURE;
  }
  err = feeds_socket_.Connect(alpha::NetAddress("10.6.224.83", 50001));
  if (err) {
    PLOG_ERROR << "Feeds socket connect failed, err: " << err;
    return EXIT_FAILURE;
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

  using namespace std::placeholders;
#define THRONES_BATTLE_REGISTER_HANDLER(ReqType, RespType, Handler) \
  message_dispatcher_.Register<ReqType, RespType>(                  \
      std::bind(&ServerApp::Handler, this, _1, _2, _3));

  THRONES_BATTLE_REGISTER_HANDLER(SignUpRequest, SignUpResponse, HandleSignUp);
  THRONES_BATTLE_REGISTER_HANDLER(QueryBattleStatusRequest,
                                  QueryBattleStatusResponse,
                                  HandleQueryBattleStatus);
  THRONES_BATTLE_REGISTER_HANDLER(PickLuckyWarriorsRequest,
                                  PickLuckyWarriorsResponse,
                                  HandlePickLuckyWarriors);
#undef THRONES_BATTLE_REGISTER_HANDLER

  if (battle_data_->SeasonFinished()) {
    AddTimerForChangeSeason();
  } else {
    AddTimerForBattleRound();
  }
  return 0;
}

int ServerApp::InitRecoveryMode() { return EXIT_FAILURE; }

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
                                   Zone* zone, CampID one, CampID the_other) {
  DLOG_INFO << "Zone: " << zone->id() << ", one camp: " << one
            << ", the other camp: " << the_other;

  auto one_camp = zone->GetCamp(one);
  auto the_other_camp = zone->GetCamp(the_other);
  auto done = [one_camp, the_other_camp] {
    return one_camp->NoLivingWarriors() || the_other_camp->NoLivingWarriors();
  };

  DLOG_INFO << "Before battle, Zone: " << zone->id()
            << ", one camp living: " << one_camp->LivingWarriorsNum()
            << ", the other camp living: "
            << the_other_camp->LivingWarriorsNum();

  if (one_camp->NoLivingWarriors() && the_other_camp->NoLivingWarriors()) {
    // 两个阵营都没人, 随机一个胜利阵营
    CampID camp_win = alpha::Random::Rand32(0, 2) % 2 == 0 ? one : the_other;
    LOG_INFO << "No warrior found on both sides, one: " << one
             << ", the_other: " << the_other << ", random winner: " << camp_win;
    DoWhenTwoCampsMatchDone(zone, one_camp, the_other_camp,
                            zone->GetCamp(camp_win));
    return;
  }

  auto warrior_fight_result_callback = std::bind(
      &ServerApp::ProcessFightTaskResult, this, std::placeholders::_1);

  for (auto match = 1; !done(); ++match) {
    auto& one_living_warriors = one_camp->living_warriors();
    auto& the_other_living_warriors = the_other_camp->living_warriors();
    auto battle_warrior_size =
        std::min(one_living_warriors.size(), the_other_living_warriors.size());
    CHECK(battle_warrior_size != 0u);
    auto one_camp_choosen_warrior_iters =
        alpha::Random::Sample(one_living_warriors.begin(),
                              one_living_warriors.end(), battle_warrior_size);
    auto the_other_camp_choosen_warrior_iters = alpha::Random::Sample(
        the_other_living_warriors.begin(), the_other_living_warriors.end(),
        battle_warrior_size);

    UinList one_camp_choosen_warriors, the_other_camp_choosen_warriors;
    one_camp_choosen_warriors.reserve(battle_warrior_size);
    the_other_camp_choosen_warriors.reserve(battle_warrior_size);
    std::transform(one_camp_choosen_warrior_iters.begin(),
                   one_camp_choosen_warrior_iters.end(),
                   std::back_inserter(one_camp_choosen_warriors),
                   [](const UinSet::const_iterator& it) { return *it; });
    std::transform(the_other_camp_choosen_warrior_iters.begin(),
                   the_other_camp_choosen_warrior_iters.end(),
                   std::back_inserter(the_other_camp_choosen_warriors),
                   [](const UinSet::const_iterator& it) { return *it; });
    TaskBroker broker(client, co, conf_->fight_server_addr(), zone->id(), one,
                      warrior_fight_result_callback);
    broker.SetOneCampWarriorRange(one_camp_choosen_warriors);
    broker.SetTheOtherCampWarriorRange(the_other_camp_choosen_warriors);
    broker.Wait();
  }

  auto winner_camp = one_camp->NoLivingWarriors() ? the_other_camp : one_camp;
  DoWhenTwoCampsMatchDone(zone, one_camp, the_other_camp, winner_camp);
}

void ServerApp::AddRoundReward(Zone* zone, Camp* camp) {
  auto zone_current_round = zone->matchups()->CurrentRound();
  auto game_log = zone->matchups()->GetGameLog(camp->id(), zone_current_round);
  auto zone_conf = conf_->GetZoneConf(zone->id());
  auto reward = zone_conf->GetRoundReward(game_log);
  // 为这个区所有人加上这份奖励
  camp->ForeachWarrior([this, reward](UinType uin) {
    auto it = rewards_->find(uin);
    if (it != rewards_->end()) {
      it->second.Merge(reward);
    } else {
      rewards_->insert(alpha::make_pod_pair(uin, reward));
    }
  });
}

void ServerApp::ProcessFightTaskResult(
    const FightServerProtocol::TaskResult& result) {
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
      ProcessSurvivedWarrior(winner, loser);
      ProcessDeadWarrior(loser, winner, *loser_view_fight_content);
    } else {
      LOG_WARNING << "Fight failed, challenger: " << challenger
                  << ", defender: " << defender
                  << ", error: " << fight_result.error();
    }
  }
}

void ServerApp::ProcessSurvivedWarrior(UinType winner, UinType loser) {
  auto it = warriors_->find(winner);
  if (it == warriors_->end()) {
    LOG_WARNING << "Cannot find warrior, winner: " << winner;
  }
  auto& warrior = it->second;
  auto zone = battle_data_->GetZone(warrior.zone_id());
  auto camp = zone->GetCamp(warrior.camp_id());
  // 更新击杀数
  warrior.add_killing_num();
  // TODO: 上报排行榜
  zone->leaders()->Notify(camp->id(), winner, warrior.killing_num());
}

void ServerApp::ProcessRoundSurvivedWarrior(UinType uin) {
  auto it = warriors_->find(uin);
  // 从Camp中取出的uin, 直接CHECK
  CHECK(it != warriors_->end()) << "Cannot find warrior, uin: " << uin;
  auto& warrior = it->second;
  auto zone = battle_data_->GetZone(warrior.zone_id());
  bool bye = warrior.last_killed_warrior() == 0;
  if (bye) {
    // 写轮空feeds
    AddFightMessage(&feeds_socket_, kThronesBattleBye, uin, 0,
                    battle_data_->CurrentSeason(),
                    zone->matchups()->CurrentRound());
  } else {
    // 写本轮胜利feeds
    AddFightMessage(
        &feeds_socket_, kThronesBattleWin, uin, warrior.last_killed_warrior(),
        battle_data_->CurrentSeason(), zone->matchups()->CurrentRound(),
        warrior.last_killed_warrior(), warrior.killing_num());
  }
}

void ServerApp::DoWhenSeasonChanged() {
  LOG_INFO << "Season changed, new season: " << battle_data_->CurrentSeason();
  // 换届干掉上一届的参战人员
  warriors_->clear();
  AddTimerForBattleRound();
}

void ServerApp::DoWhenSeasonFinished() {
  LOG_INFO << "Season " << battle_data_->CurrentSeason() << " finished";
  battle_data_->SetSeasonFinished();
  AddTimerForChangeSeason();
}

void ServerApp::DoWhenRoundFinished() {
  CHECK(battle_data_->CurrentRoundFinished());
  auto current = battle_data_->CurrentRound();
  LOG_INFO << "Round " << current << " finished";
  if (current == kMaxRoundID) {
    DoWhenSeasonFinished();
  } else {
    AddTimerForBattleRound();
  }
}

void ServerApp::DoWhenTwoCampsMatchDone(Zone* zone, Camp* one, Camp* the_other,
                                        Camp* winner_camp) {
  CHECK(winner_camp == one || winner_camp == the_other);
  LOG_INFO << "Battle done, Zone: " << zone->id() << " one: " << one->id()
           << ", the_other: " << the_other->id()
           << ", winner camp: " << winner_camp->id();

  // 更新对战结果
  zone->matchups()->SetBattleResult(one->id(), one == winner_camp,
                                    one->LivingWarriorsNum());
  zone->matchups()->SetBattleResult(the_other->id(), the_other == winner_camp,
                                    the_other->LivingWarriorsNum());
  // 为双方阵营加本轮奖励
  AddRoundReward(zone, one);
  AddRoundReward(zone, the_other);

  // 为获胜阵营所有仍然存活的人写feeds
  auto& living_warriors = winner_camp->living_warriors();
  for (auto uin : living_warriors) {
    ProcessRoundSurvivedWarrior(uin);
  }

  // 如果打完了最后一轮，根据名次写feeds
  unsigned one_rank = zone->matchups()->FinalRank(one->id());
  if (one_rank != 0) {
    CHECK(one_rank <= kCampIDMax);
    for (const auto& uin : one->Warriors()) {
      AddFightMessage(&feeds_socket_, kThronesBattleCampRank[one_rank - 1], uin,
                      0, battle_data_->CurrentSeason(), zone->id(), one_rank);
    }
  }
  unsigned the_other_rank = zone->matchups()->FinalRank(the_other->id());
  if (the_other_rank != 0) {
    CHECK(the_other_rank <= kCampIDMax);
    for (const auto& uin : the_other->Warriors()) {
      AddFightMessage(
          &feeds_socket_, kThronesBattleCampRank[the_other_rank - 1], uin, 0,
          battle_data_->CurrentSeason(), zone->id(), the_other_rank);
    }
  }

  // 判断当前所有赛区本轮所有比赛是否已经打完
  if (battle_data_->CurrentRoundFinished()) {
    DoWhenRoundFinished();
  }
}

void ServerApp::ProcessDeadWarrior(UinType loser, UinType winner,
                                   const std::string& fight_content) {
  auto it = warriors_->find(loser);
  CHECK(it != warriors_->end()) << "Cannot find warrior, loser: " << loser;
  auto zone = battle_data_->GetZone(it->second.zone_id());
  auto camp = zone->GetCamp(it->second.camp_id());
  it->second.set_dead(true);
  camp->MarkWarriorDead(loser);
  // 写战败的feeds
  AddFightEvent(&feeds_socket_, kThronesBattleLose, loser, winner,
                fight_content, battle_data_->CurrentSeason(),
                zone->matchups()->CurrentRound(), winner,
                it->second.killing_num());
}

void ServerApp::AddTimerForChangeSeason() {
  CHECK(battle_data_->SeasonFinished());
  alpha::TimeStamp at = alpha::from_time_t(conf_->NextSeasonBaseTime());
  DLOG_INFO << "Will change season at " << at;
  loop_.RunAt(at, [this] {
    bool done = battle_data_->ChangeSeason();
    if (done) {
      DoWhenSeasonChanged();
    } else {
      LOG_WARNING << "Change season failed";
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
  DLOG_INFO << "Will start round " << next_round << " at " << at;
  loop_.RunAt(at, [this] { RunBattle(); });
}

void ServerApp::InitBeforeNewSeasonBattle() {
  CHECK(battle_data_->SeasonNotStarted());
  DLOG_INFO << "Reset season data";
  battle_data_->ResetSeasonData();
}

void ServerApp::RunBattle() {
  if (battle_data_->SeasonNotStarted()) {
    InitBeforeNewSeasonBattle();
    battle_data_->SetCurrentRound(1);
  }
  CHECK(!battle_data_->SeasonFinished());
  auto current_round = battle_data_->CurrentRound();
  for (uint16_t zone_id = kMinZoneID; zone_id <= kMaxZoneID; ++zone_id) {
    auto zone = battle_data_->GetZone(zone_id);
    auto zone_current_round = zone->matchups()->CurrentRound();
    if (zone_current_round == current_round) {
      // 已经开始本轮
    } else if (zone_current_round == current_round - 1) {
      // 开启本轮
      bool ok = zone->matchups()->StartNextRound();
      CHECK(ok) << "Start next round failed, zone_id: " << zone_id;
      DLOG_INFO << "Set zone(" << zone_id << ") to next round("
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

  // 获得每个赛区当前轮没有完成的战斗对
  using BattleTaskType = std::tuple<Zone*, CampID, CampID>;
  std::vector<BattleTaskType> unfinished_battle;
  battle_data_->ForeachZone([&unfinished_battle, current_round](Zone& zone) {
    auto fill = [&unfinished_battle, &zone](CampID one, CampID the_other) {
      unfinished_battle.push_back(std::make_tuple(&zone, one, the_other));
    };
    zone.matchups()->UnfinishedBattle(current_round, fill);
  });

  DLOG_INFO << "Round " << current_round << " got " << unfinished_battle.size()
            << " battle(s)";
  if (!unfinished_battle.empty()) {
    // 还有没完成的战斗对
    std::for_each(std::begin(unfinished_battle), std::end(unfinished_battle),
                  [this](BattleTaskType& task) {
      using namespace std::placeholders;
      async_tcp_client_.RunInCoroutine(
          std::bind(&ServerApp::RoundBattleRoutine, this, _1, _2,
                    std::get<0>(task), std::get<1>(task), std::get<2>(task)));
    });
  } else if (current_round == kMaxRoundID) {
    // 已经打完最后一轮
    DoWhenSeasonFinished();
  } else {
    // 准备打下一轮
    battle_data_->SetCurrentRound(current_round + 1);
    AddTimerForBattleRound();
  }
}

bool ServerApp::RecoveryMode() const {
  auto env = getenv("RecoveryMode");
  if (env) {
    std::string mode(env);
    if (mode == "1" || boost::iequals(mode, "true")) {
      return true;
    }
  }
  return false;
}

int ServerApp::Run() {
  /* Daemonize */
  // int err = daemon(0, 0);
  // if (err) {
  //  PLOG_ERROR << "daemon";
  //  return err;
  //}
  /* Trap signals */
  TrapSignals();
  using namespace std::placeholders;
  udp_server_.SetMessageCallback(
      std::bind(&ServerApp::HandleUDPMessage, this, _1, _2, _3, _4));
  bool ok = udp_server_.Run(alpha::NetAddress("0.0.0.0", 51000));
  if (!ok) return EXIT_FAILURE;
  loop_.Run();
  return EXIT_SUCCESS;
}

void ServerApp::HandleUDPMessage(alpha::UDPSocket* socket, alpha::IOBuffer* buf,
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

  ResponseWrapper response_wrapper;
  response_wrapper.set_ctx(request_wrapper.ctx());
  response_wrapper.set_uin(request_wrapper.uin());
  auto rc = message_dispatcher_.Dispatch(request_wrapper.uin(), message.get(),
                                         &response_wrapper);
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

int ServerApp::HandleSignUp(UinType uin, const SignUpRequest* req,
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
  std::sort(zones.begin(), zones.end(),
            [this](const Zone* lhs, const Zone* rhs) {
    auto lhs_conf = conf_->GetZoneConf(lhs);
    auto rhs_conf = conf_->GetZoneConf(rhs);
    return lhs_conf->max_level() < rhs_conf->max_level();
  });
  auto user_level = req->level();
  auto it = std::find_if(zones.begin(), zones.end(),
                         [this, user_level](const Zone* zone) {
    auto zone_conf = conf_->GetZoneConf(zone);
    return user_level <= zone_conf->max_level();
  });
  if (it == zones.end()) {
    return Error::kServerInternalError;
  }
  Zone* zone = nullptr;
  Camp* camp = nullptr;
  while (it != zones.end()) {
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
    auto camps = alpha::Random::Sample(candidate_camps.begin(),
                                       candidate_camps.end(), 1);
    CHECK(camps.size() == 1u);
    camp = *camps.front();
    break;
  }
  if (camp == nullptr) {
    return Error::kAllZoneAreFull;
  }
  auto p = warriors_->insert(
      alpha::make_pod_pair(uin, Warrior::Create(uin, zone->id(), camp->id())));
  CHECK(p.second) << "Insert new warrior failed";
  camp->AddWarrior(uin);
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
    camp_proto->set_id(camp->id());
    camp_proto->set_warriors(camp->WarriorsNum());
    // 规则:
    //  1.查看当前正在打的这轮，从camps中提取实时剩余人数
    //  2.查看已经打完的某一轮，从matchups里面提取历史记录
    //  TODO: 初始赛季有BUG
    if (!battle_data_->CurrentRoundFinished() &&
        battle_data_->CurrentRound() == battle_round) {
      camp_proto->set_living_warriors(camp->LivingWarriorsNum());
    } else {
      camp_proto->set_living_warriors(
          zone->matchups()->GetFinalLivingWarriorsNum(battle_round,
                                                      camp->id()));
    }
    auto leader = zone->leaders()->GetLeader(camp->id());
    if (!leader.empty()) {
      camp_proto->set_leader(leader.uin);
      camp_proto->set_leader_killing_num(leader.killing_num);
      camp_proto->set_picked_lucky_warriors(leader.picked_lucky_warriors);
    }
    auto lucky_warriors = zone->lucky_warriors()->Get(camp->id());
    auto lucky_warriors_proto = resp->add_lucky_warriors();
    lucky_warriors_proto->set_camp(camp->id());
    std::copy(std::begin(lucky_warriors), std::end(lucky_warriors),
              google::protobuf::RepeatedFieldBackInserter(
                  lucky_warriors_proto->mutable_lucky_warrior()));
  }
  auto matchups = zone->matchups();
  auto matchups_proto = resp->mutable_matchups();
  matchups_proto->set_zone(zone->id());
  auto cb = [matchups_proto](CampID one, CampID the_other) {
    auto round_matchups = matchups_proto->add_round_matchups();
    round_matchups->set_one_camp(one);
    round_matchups->set_the_other_camp(the_other);
  };
  matchups->ForeachMatchup(battle_round, cb);
  auto current_season = battle_data_->CurrentSeason();
  CHECK(current_season >= 1);
  if (battle_data_->SeasonFinished()) {
    resp->set_lucky_warriors_season(current_season);
  } else if (current_season == 1) {
    resp->set_lucky_warriors_season(1);  // 第一届还没打完，不能显示第0届啊
  } else {
    resp->set_lucky_warriors_season(current_season - 1);
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
    UinType uin, const QueryLuckyWarriorRewardRequest* req,
    QueryLuckyWarriorRewardResponse* resp) {
  if (unlikely(uin == 0)) {
    return Error::kInvalidArgument;
  }
  // TODO: 太他妈丑了
  bool is_lucky_warrior = false;
  battle_data_->ForeachZone([uin, &is_lucky_warrior](const Zone& zone) {
    if (is_lucky_warrior) return;
    for (auto i = 0; i < kCampIDMax; ++i) {
      if (zone.lucky_warriors()->HasWarrior(uin)) {
        is_lucky_warrior = true;
        break;
      }
    }
  });
  if (!is_lucky_warrior) {
    return Error::kNotLuckyWarrior;
  }
  auto reward = conf_->lucky_warrior_reward();
  auto reward_proto = resp->mutable_reward();
  reward_proto->set_battle_point(reward.battle_point);
  for (auto i = 0; i < reward.goods_size(); ++i) {
    auto goods_proto = reward_proto->add_goods();
    goods_proto->set_id(reward.goods_reward[i].id);
    goods_proto->set_num(reward.goods_reward[i].num);
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
  std::transform(generals.begin(), generals.end(),
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
}
