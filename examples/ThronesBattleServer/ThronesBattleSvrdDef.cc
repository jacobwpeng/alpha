/*
 * =============================================================================
 *
 *       Filename:  ThronesBattleSvrdDef.cc
 *        Created:  12/19/15 09:11:51
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "ThronesBattleSvrdDef.h"
#include <cstring>
#include <algorithm>
#include <alpha/logger.h>
#include <alpha/random.h>

namespace ThronesBattle {
bool ValidCampID(CampID camp_id) {
  return camp_id >= CampID::kMin && camp_id <= CampID::kMax;
}

bool ValidZoneID(uint16_t zone_id) {
  return zone_id >= 1 && zone_id <= kCurrentZoneNum;
}

void LuckyWarriorList::Clear() {
  memset(lucky_warriros_, 0x0, sizeof(lucky_warriros_));
}

void CampMatchups::Reset() {
  current_battle_round_ = 0;
  memset(matchups_data_, 0x0, sizeof(matchups_data_));
  auto camp = CampID::kMin;
  while (camp <= CampID::kMax) {
    auto index = (uint16_t)camp - 1;
    matchups_data_[0][index].camp = camp;
    camp = (CampID)((uint16_t)camp + 1);
  }
}

void CampMatchups::SetBattleResult(CampID camp, bool win,
                                   uint32_t final_live_warriors_num) {
  auto matchup_data = FindMatchupData(current_battle_round_, camp);
  matchup_data->win = win;
  matchup_data->final_live_warriors_num = final_live_warriors_num;
  matchup_data->set = true;
}

bool CampMatchups::BattleNotStarted() const {
  return current_battle_round_ == 0;
}

bool CampMatchups::CurrentRoundFinished() const {
  return RoundFinished(current_battle_round_);
}

bool CampMatchups::RoundFinished(uint16_t battle_round) const {
  CHECK(battle_round != 0 && battle_round <= kMaxRoundID);
  auto index = battle_round - 1;
  return std::all_of(std::begin(matchups_data_[index]),
                     std::end(matchups_data_[index]),
                     [](const MatchupData& d) { return d.set; });
}

uint16_t CampMatchups::CurrentRound() const { return current_battle_round_; }

std::string CampMatchups::GetGameLog(CampID camp,
                                     uint16_t max_battle_round) const {
  CHECK(max_battle_round <= current_battle_round_);
  std::string game_log;
  for (uint16_t round = 1; round <= max_battle_round; ++round) {
    auto matchup_data = FindMatchupData(round, camp);
    CHECK(matchup_data && matchup_data->set);
    game_log += matchup_data->win ? '1' : '0';
  }
  return game_log;
}

void CampMatchups::ForeachMatchup(uint16_t battle_round, MatchupFunc func) {
  CHECK(battle_round != 0 && battle_round < kMaxRoundID);
  CHECK(RoundFinished(battle_round));
  for (auto i = 0; i < kCampIDMax - 1; i += 2) {
    func(matchups_data_[battle_round - 1][i].camp,
         matchups_data_[battle_round - 1][i + 1].camp);
  }
}

void CampMatchups::UnfinishedBattle(uint16_t battle_round, MatchupFunc func) {
  CHECK(battle_round != 0 && battle_round <= kMaxRoundID);
  auto index = battle_round - 1;
  for (auto i = 0; i < kCampIDMax - 1; i += 2) {
    auto& matchup_data = matchups_data_[index][i];
    if (!matchup_data.set) {
      func(matchup_data.camp, matchups_data_[index][i + 1].camp);
    }
  }
}

bool CampMatchups::StartNextRound() {
  if (current_battle_round_ == kMaxRoundID) {
    // No more round
    return false;
  }
  if (current_battle_round_ != 0 && !RoundFinished(current_battle_round_)) {
    // Current round not finished
    return false;
  }

  // No common way to do it...
  static_assert(kCampIDMax == 8, "Cannot handle camps more thatn 8");
  if (current_battle_round_ == 0) {
    // Shuffle all camps
    alpha::Random::Shuffle(std::begin(matchups_data_[0]),
                           std::end(matchups_data_[0]));
  } else if (current_battle_round_ == 1) {
    // Camps are devided into 2 groups, in each group winners of last round
    // fight each other
    std::copy(std::begin(matchups_data_[0]), std::end(matchups_data_[0]),
              matchups_data_[1]);
    auto begin = std::begin(matchups_data_[1]);
    auto end = std::next(begin, 4);
    auto cmp = [](const MatchupData& d) { return d.win; };
    std::stable_partition(begin, end, cmp);
    std::stable_partition(end, std::end(matchups_data_[1]), cmp);
    std::for_each(std::begin(matchups_data_[current_battle_round_]),
                  std::end(matchups_data_[current_battle_round_]),
                  [](MatchupData& d) {
      d.set = false;
      d.win = false;
      d.final_live_warriors_num = 0;
    });
  } else if (current_battle_round_ == 2) {
    // Match between groups by game_log
    // ig:  win win <-> win win
    //      win lose <-> win lose
    //      lose win <-> lose win
    //      lose lose <-> lose lose
    using TupleType = std::tuple<std::string, int, CampID>;
    std::set<TupleType, std::greater<TupleType> > s;
    for (auto i = 0; i < kCampIDMax; ++i) {
      auto camp = matchups_data_[1][i].camp;
      auto game_log = GetGameLog(camp, 2);
      s.insert(TupleType(game_log, kCampIDMax - i, camp));
    }
    std::transform(s.begin(), s.end(), matchups_data_[2],
                   [](const TupleType& p) {
      MatchupData d;
      d.camp = std::get<2>(p);
      d.final_live_warriors_num = 0;
      d.set = false;
      return d;
    });
  }

  ++current_battle_round_;
  return true;
}

CampMatchups::MatchupData* CampMatchups::FindMatchupData(uint16_t battle_round,
                                                         CampID camp) {
  DLOG_INFO << "battle round: " << battle_round << ", camp: " << camp;
  CHECK(battle_round != 0 && battle_round <= kMaxRoundID)
      << "Invalid battle round: " << battle_round;
  auto index = battle_round - 1;
  auto it = std::find_if(
      std::begin(matchups_data_[index]), std::end(matchups_data_[index]),
      [&camp](const MatchupData& d) { return d.camp == camp; });
  CHECK(it != std::end(matchups_data_[index]));
  return &*it;
}

const CampMatchups::MatchupData* CampMatchups::FindMatchupData(
    uint16_t battle_round, CampID camp) const {
  return const_cast<CampMatchups*>(this)->FindMatchupData(battle_round, camp);
}

Camp::Camp(CampID id) : id_(id) {}

void Camp::AddWarrior(UinType uin, bool dead) {
  auto p = warriors_.insert(uin);
  CHECK(p.second);
  if (!dead) {
    live_warriors_.insert(uin);
  }
}

void Camp::SetWarriorDead(UinType uin) { live_warriors_.erase(uin); }

std::vector<UinType> Camp::LivingWarriors() const {
  return std::vector<UinType>(live_warriors_.begin(), live_warriors_.end());
}

bool operator<(const Camp& lhs, const Camp& rhs) { return lhs.id() < rhs.id(); }

Zone::Zone(uint16_t id, CampMatchups* matchups,
           CampMatchups* last_season_matchups)
    : id_(id),
      matchups_(matchups),
      last_season_matchups_(last_season_matchups) {
  for (auto camp_id = CampID::kMin; camp_id <= CampID::kMax;) {
    camps_.emplace_back(camp_id);
    camp_id = (CampID)((size_t)camp_id + 1);
  }
}

Camp* Zone::GetCamp(CampID camp_id) {
  CHECK(ValidCampID(camp_id));
  auto index = (size_t)camp_id;
  CHECK(index <= camps_.size());
  return &camps_[index - 1];
}

BattleDataSaved* BattleDataSaved::Create(void* p, size_t sz) {
  if (sz < sizeof(BattleDataSaved)) {
    return nullptr;
  }
  memset(p, 0x0, sizeof(BattleDataSaved));
  auto d = reinterpret_cast<BattleDataSaved*>(p);
  d->magic = kMagic;
  d->last_season_data_dropped = false;
  d->season_finished_ = false;
  d->battle_round = 0;
  d->last_battle_season = 0;
  d->battle_season = 0;
  // d->General_in_chief_list.Clear();
  d->lucky_warriors.Clear();
  for (auto i = 0; i < kMaxZoneNum; ++i) {
    d->last_season_matchups[i].Reset();
    d->matchups[i].Reset();
  }
  return d;
}

BattleDataSaved* BattleDataSaved::Restore(void* p, size_t sz) {
  if (sz < sizeof(BattleDataSaved)) {
    return nullptr;
  }
  auto d = reinterpret_cast<BattleDataSaved*>(p);
  if (d->magic != kMagic) {
    return nullptr;
  }
  return d;
}

BattleData::BattleData(BattleDataSaved* d) : battle_data_saved_(d) {
  CHECK(d);
  for (uint16_t zone_id = kMinZoneID; zone_id <= kMaxZoneID; ++zone_id) {
    auto matchups = &battle_data_saved_->matchups[zone_id - 1];
    auto last_season_matchups =
        &battle_data_saved_->last_season_matchups[zone_id - 1];
    zones_.emplace_back(zone_id, matchups, last_season_matchups);
  }
}

bool BattleData::last_season_data_dropped() const {
  return battle_data_saved_->last_season_data_dropped;
}

void BattleData::set_last_season_data_dropped(bool dropped) {
  battle_data_saved_->last_season_data_dropped = dropped;
}

Zone* BattleData::GetZone(uint16_t zone_id) {
  CHECK(ValidZoneID(zone_id));
  return &zones_[zone_id - 1];
}

bool BattleData::ChangeSeason() {
  if (!SeasonFinished()) {
    return false;
  }
  battle_data_saved_->last_season_data_dropped = false;
  battle_data_saved_->battle_round = 0;
  battle_data_saved_->last_battle_season = battle_data_saved_->battle_season;
  ++battle_data_saved_->battle_season;
  std::copy(std::begin(battle_data_saved_->matchups),
            std::end(battle_data_saved_->matchups),
            battle_data_saved_->last_season_matchups);
  std::for_each(std::begin(battle_data_saved_->matchups),
                std::end(battle_data_saved_->matchups),
                [](CampMatchups& matchups) { matchups.Reset(); });
  return true;
}

void BattleData::DropLastSeasonData() {
  battle_data_saved_->lucky_warriors.Clear();
}

void BattleData::SetCurrentRound(uint16_t battle_round) {
  CHECK(battle_round != 0 && battle_round <= kMaxRoundID);
  battle_data_saved_->battle_round = battle_round;
}

void BattleData::SetSeasonFinished() {
  battle_data_saved_->season_finished_ = true;
}

bool BattleData::SeasonFinished() const {
  return battle_data_saved_->season_finished_;
}

bool BattleData::SeasonNotStarted() const {
  return battle_data_saved_->battle_round == 0;
}

uint16_t BattleData::CurrentRound() const {
  return battle_data_saved_->battle_round;
}

bool BattleData::CurrentRoundFinished() const {
  bool finished = true;
  ForeachZone([&finished](const Zone& zone) {
    finished = finished && zone.matchups()->CurrentRoundFinished();
  });
  return finished;
}

Reward Reward::Create(uint32_t battle_point) {
  Reward reward;
  reward.Clear();
  return reward;
}

void Reward::Clear() {
  next_goods_reward_index = 0;
  battle_point = 0;
  memset(goods_reward, 0x0, sizeof(goods_reward));
}

bool Reward::AddGoods(uint32_t goods_id, uint32_t goods_num) {
  for (uint32_t i = 0; i < next_goods_reward_index; ++i) {
    if (goods_reward[i].goods_id == goods_id) {
      goods_reward[i].goods_num += goods_num;
      return true;
    }
  }
  if (next_goods_reward_index == kMaxGoodsRewardKind) {
    return false;
  }
  goods_reward[next_goods_reward_index].goods_id = goods_id;
  goods_reward[next_goods_reward_index].goods_num = goods_num;
  ++next_goods_reward_index;
  return true;
}

bool Reward::Empty() const {
  return next_goods_reward_index == 0 && battle_point == 0;
}

ZoneConf::ZoneConf(uint16_t id, unsigned max_camp_warriors_num,
                   unsigned max_level, const std::string& name)
    : id_(id),
      max_camp_warriors_num_(max_camp_warriors_num),
      max_level_(max_level),
      name_(name) {}

void ZoneConf::AddRoundReward(const std::string& game_log,
                              const Reward& reward) {
  round_reward_map_.insert(std::make_pair(game_log, reward));
}

void ZoneConf::AddRankReward(unsigned max_rank, const Reward& reward) {
  rank_reward_map_.insert(std::make_pair(max_rank, reward));
}
}
