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
#include <alpha/logger.h>
#include <alpha/random.h>
#include <boost/algorithm/string/predicate.hpp>
#include "ThronesBattleSvrdConf.h"
#include "ThronesBattleSvrdTaskBroker.h"

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
ServerApp::ServerApp() : async_tcp_client_(&loop_) {}

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

  return RecoveryMode() ? InitRemoveryMode() : InitNormalMode();
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

  if (battle_data_->SeasonFinished()) {
    AddTimerForChangeSeason();
  } else if (!battle_data_->last_season_data_dropped()) {
    AddTimerForDropLastSeasonData();
  } else {
    AddTimerForBattleRound();
  }
  return 0;
}

int ServerApp::InitRemoveryMode() { return EXIT_FAILURE; }

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
    return one_camp->LivingWarriors().empty() ||
           the_other_camp->LivingWarriors().empty();
  };

  DLOG_INFO << "Before battle, Zone: " << zone->id()
            << ", one camp living: " << one_camp->LivingWarriors().size()
            << ", the other camp living: "
            << the_other_camp->LivingWarriors().size();

  auto check_when_battle_done = [this] {
    if (battle_data_->CurrentRoundFinished()) {
      auto current_round = battle_data_->CurrentRound();
      LOG_INFO << "Round " << current_round << " finished";
      if (current_round == kMaxRoundID) {
        LOG_INFO << "Season finished";
        battle_data_->SetSeasonFinished();
        AddTimerForChangeSeason();
      } else {
        battle_data_->SetCurrentRound(current_round + 1);
        AddTimerForBattleRound();
      }
    }
  };

  if (one_camp->LivingWarriors().empty() &&
      the_other_camp->LivingWarriors().empty()) {
    // Both camp are empty
    CampID camp_win = alpha::Random::Rand32() % 2 == 0 ? one : the_other;
    DLOG_INFO << "No warrior found on both sides, one: " << one
              << ", the_other: " << the_other
              << ", random winner: " << camp_win;
    zone->matchups()->SetBattleResult(one, camp_win == one, 0);
    zone->matchups()->SetBattleResult(the_other, camp_win == the_other, 0);
    check_when_battle_done();
    return;
  }

  auto warrior_fight_result_callback = [this](
      const FightServerProtocol::TaskResult& result) {
    for (const auto& fight_result : result.fight_pair_result()) {
      if (fight_result.error() == 0) {
        UinType winner = fight_result.winner();
        UinType loser = winner == fight_result.challenger()
                            ? fight_result.defender()
                            : fight_result.challenger();
        MarkWarriorDead(loser);
      }
    }
  };

  while (!done()) {
    std::vector<UinType> one_living_warriors = one_camp->LivingWarriors();
    std::vector<UinType> the_other_living_warriors =
        the_other_camp->LivingWarriors();
    alpha::Random::Shuffle(one_living_warriors.begin(),
                           one_living_warriors.end());
    alpha::Random::Shuffle(the_other_living_warriors.begin(),
                           the_other_living_warriors.end());
    auto battle_warrior_size =
        std::min(one_living_warriors.size(), the_other_living_warriors.size());
    CHECK(battle_warrior_size != 0u);
    one_living_warriors.resize(battle_warrior_size);
    the_other_living_warriors.resize(battle_warrior_size);
    TaskBroker broker(client, co, one_living_warriors,
                      the_other_living_warriors, warrior_fight_result_callback);
    broker.Wait();
  }

  DLOG_INFO << "Battle done, Zone: " << zone->id()
            << ", one camp living: " << one_camp->LivingWarriors().size()
            << ", the other camp living: "
            << the_other_camp->LivingWarriors().size();

  CampID camp_win = one_camp->LivingWarriors().empty() ? the_other : one;

  zone->matchups()->SetBattleResult(one, camp_win == one,
                                    one_camp->LivingWarriors().size());
  zone->matchups()->SetBattleResult(the_other, camp_win == the_other,
                                    the_other_camp->LivingWarriors().size());
  check_when_battle_done();
}

void ServerApp::MarkWarriorDead(UinType uin) {
  auto it = warriors_->find(uin);
  CHECK(it != warriors_->end()) << "Cannot find warrior, uin: " << uin;
  auto zone = battle_data_->GetZone(it->second.zone_id());
  auto camp = zone->GetCamp(it->second.camp_id());
  camp->SetWarriorDead(uin);
}

void ServerApp::AddTimerForChangeSeason() {
  CHECK(battle_data_->SeasonFinished());
  alpha::TimeStamp at = alpha::from_time_t(conf_->NextSeasonBaseTime());
  DLOG_INFO << "Will change season at " << at;
  loop_.RunAt(at, [this] {
    bool done = battle_data_->ChangeSeason();
    if (done) {
      DLOG_INFO << "Season changed, last: " << battle_data_->LastSeason()
                << "current: " << battle_data_->CurrentSeason();
      AddTimerForDropLastSeasonData();
    } else {
      LOG_WARNING << "Change season failed";
    }
  });
}

void ServerApp::AddTimerForDropLastSeasonData() {
  bool season_finished = battle_data_->SeasonFinished();
  alpha::TimeStamp at =
      alpha::from_time_t(conf_->NextDropLastSeasonDataTime(season_finished));
  DLOG_INFO << "Will drop last season data at " << at;
  loop_.RunAt(at, [this] {
    battle_data_->DropLastSeasonData();
    DLOG_INFO << "Last season data dropped";
    auto current_round = battle_data_->CurrentRound();
    CHECK(current_round == 0);
    battle_data_->SetCurrentRound(1);
    DLOG_INFO << "Set current round 1";
    AddTimerForBattleRound();
  });
}

void ServerApp::AddTimerForBattleRound() {
  CHECK(!battle_data_->SeasonFinished());
  auto current_round = battle_data_->CurrentRound();
  alpha::TimeStamp at =
      alpha::from_time_t(conf_->BattleRoundStartTime(current_round));
  DLOG_INFO << "Will start round " << current_round << " at " << at;
  loop_.RunAt(at, [this] { RunBattle(); });
}

void ServerApp::RunBattle() {
  CHECK(!battle_data_->SeasonFinished());
  auto current_round = battle_data_->CurrentRound();
  CHECK(current_round != 0);
  battle_data_->ForeachZone([current_round](Zone& zone) {
    auto zone_current_round = zone.matchups()->CurrentRound();
    CHECK(zone_current_round == current_round ||
          zone_current_round == current_round - 1)
        << "Invalid zone current round: " << zone_current_round
        << ", current round: " << current_round;
    if (zone_current_round == current_round - 1) {
      bool ok = zone.matchups()->StartNextRound();
      CHECK(ok);
      DLOG_INFO << "Set zone(" << zone.id() << ") to round "
                << zone.matchups()->CurrentRound();
    }
  });

  // Get all unfinished battle of current round
  using BattleTaskType = std::tuple<Zone*, CampID, CampID>;
  std::vector<BattleTaskType> unfinished_battle;
  battle_data_->ForeachZone([&unfinished_battle, current_round](Zone& zone) {
    auto fill = [&unfinished_battle, &zone](CampID one, CampID the_other) {
      unfinished_battle.push_back(std::make_tuple(&zone, one, the_other));
    };
    zone.matchups()->UnfinishedBattle(current_round, fill);
  });

  DLOG_INFO << "Round " << current_round << " got " << unfinished_battle.size()
            << " unfinished battle";
  if (!unfinished_battle.empty()) {
    std::for_each(std::begin(unfinished_battle), std::end(unfinished_battle),
                  [this](BattleTaskType& task) {
      using namespace std::placeholders;
      async_tcp_client_.RunInCoroutine(
          std::bind(&ServerApp::RoundBattleRoutine, this, _1, _2,
                    std::get<0>(task), std::get<1>(task), std::get<2>(task)));
    });
  } else {
    if (current_round == kMaxRoundID) {
      battle_data_->SetSeasonFinished();
    } else {
      auto next_round = current_round + 1;
      battle_data_->SetCurrentRound(next_round);
      AddTimerForBattleRound();
    }
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
  loop_.Run();
  return EXIT_SUCCESS;
}
}
