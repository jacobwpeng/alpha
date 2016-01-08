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
bool ValidCampID(uint16_t camp_id) {
  return camp_id >= CampID::kMin && camp_id <= CampID::kMax;
}

bool ValidZoneID(uint16_t zone_id) {
  return zone_id >= 1 && zone_id <= kCurrentZoneNum;
}

void GeneralInChiefList::AddGeneralInChief(CampID camp, UinType uin,
                                           uint32_t season) {
  CHECK(next_index_ < kMaxGeneralNum);
  generals_[next_index_] = {.camp = camp, .uin = uin, .season = season};
  ++next_index_;
}

std::vector<General> GeneralInChiefList::Get(unsigned start,
                                             unsigned num) const {
  std::vector<General> result;
  for (uint16_t i = start; i < next_index_ && result.size() < num; ++i) {
    result.push_back(generals_[i]);
  }
  return result;
}

void GeneralInChiefList::Clear() {
  next_index_ = 0;
  memset(generals_, 0x0, sizeof(generals_));
}

void LuckyWarriorList::AddLuckyWarrior(CampID camp, UinType uin) {
  uint16_t index = camp - 1;
  auto begin = std::begin(lucky_warriors_[index]);
  auto end = std::end(lucky_warriors_[index]);
  auto it = std::find(begin, end, 0);
  CHECK(it != end);
  *it = uin;
}

std::vector<UinType> LuckyWarriorList::Get(uint16_t camp_id) const {
  CHECK(ValidCampID(camp_id));
  uint16_t index = camp_id - 1;
  auto begin = std::begin(lucky_warriors_[index]);
  auto end = std::end(lucky_warriors_[index]);
  return std::vector<UinType>(begin, end);
}

bool LuckyWarriorList::HasWarrior(UinType uin) const {
  for (auto i = 0; i < kCampIDMax; ++i) {
    auto b = std::begin(lucky_warriors_[i]);
    auto e = std::end(lucky_warriors_[i]);
    if (std::find(b, e, uin) != e) {
      return true;
    }
  }
  return false;
}

bool LuckyWarriorList::PickedLuckyWarriors(CampID camp_id) const {
  // 所有Uin都为0，视为没有挑选
  return !std::all_of(std::begin(lucky_warriors_[camp_id - 1]),
                      std::end(lucky_warriors_[camp_id - 1]),
                      [](UinType uin) { return uin == 0; });
}

void LuckyWarriorList::Clear() {
  memset(lucky_warriors_, 0x0, sizeof(lucky_warriors_));
}

CampLeader CampLeaderList::GetLeader(CampID camp_id) {
  return *GetLeaderPtr(camp_id);
}

void CampLeaderList::SetPickedLuckyWarriors(CampID camp_id) {
  auto leader = GetLeaderPtr(camp_id);
  CHECK(!leader->empty());
  leader->picked_lucky_warriors = true;
}

void CampLeaderList::Clear() { memset(leaders_, 0x0, sizeof(leaders_)); }

void CampLeaderList::Notify(CampID camp_id, UinType uin, uint32_t killing_num) {
  auto leader = GetLeaderPtr(camp_id);
  if (leader->empty() || leader->killing_num < killing_num) {
    leader->uin = uin;
    leader->killing_num = killing_num;
  }
}

CampLeader* CampLeaderList::GetLeaderPtr(CampID camp_id) {
  CHECK(ValidCampID(camp_id));
  return &leaders_[camp_id - 1];
}

void MatchupData::Clear() {
  ClearExceptCamp();
  camp = (CampID)0;
}

void MatchupData::ClearExceptCamp() {
  win = false;
  set = false;
  final_living_warriors_num = 0;
}

void MatchupData::UpdateBattleInfo(bool win,
                                   uint32_t final_living_warriors_num) {
  this->win = win;
  this->final_living_warriors_num = final_living_warriors_num;
  set = true;
}

MatchupData* CampMatchups::MatchupDataForRound(uint16_t battle_round) {
  CHECK(battle_round != 0 && battle_round <= kMaxRoundID);
  return matchups_data_[battle_round - 1];
}

void CampMatchups::Reset() {
  for (uint16_t round = 0; round < kMaxRoundID; ++round) {
    for (uint16_t camp = 0; camp < kCampIDMax; ++camp) {
      matchups_data_[round][camp].Clear();
    }
  }
  current_battle_round_ = 0;
  // Initial all camps at round 1
  for (uint16_t camp = 0; camp < kCampIDMax; ++camp) {
    matchups_data_[0][camp].camp = (CampID)(camp + 1);
  }
}

void CampMatchups::SetBattleResult(CampID camp, bool win,
                                   uint32_t final_living_warriors_num) {
  auto matchup_data = FindMatchupData(current_battle_round_, camp);
  matchup_data->UpdateBattleInfo(win, final_living_warriors_num);
  matchup_data->set = true;
}

bool CampMatchups::BattleNotStarted() const {
  return current_battle_round_ == 0;
}

bool CampMatchups::CurrentRoundFinished() const {
  return RoundFinished(current_battle_round_);
}

bool CampMatchups::RoundFinished(uint16_t battle_round) const {
  if (battle_round == 0) return true;
  CHECK(battle_round != 0 && battle_round <= kMaxRoundID);
  return std::all_of(RoundMatchupsDataBegin(battle_round),
                     RoundMatchupsDataEnd(battle_round),
                     [](const MatchupData& d) { return d.set; });
}

uint16_t CampMatchups::CurrentRound() const { return current_battle_round_; }

uint16_t CampMatchups::FinishedRound() const {
  if (CurrentRound() == 0) {
    return 0;
  } else if (CurrentRoundFinished()) {
    return CurrentRound();
  } else {
    return CurrentRound() - 1;
  }
}

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

uint32_t CampMatchups::GetFinalLivingWarriorsNum(uint16_t battle_round,
                                                 CampID camp) const {
  auto d = FindMatchupData(battle_round, camp);
  CHECK(d->set);
  return d->final_living_warriors_num;
}

uint16_t CampMatchups::FinalRank(CampID camp) const {
  if (CurrentRound() < kMaxRoundID) {
    return 0;
  }
  auto d = FindMatchupData(kMaxRoundID, camp);
  if (!d->set) {
    return 0;
  }
  for (auto i = 0; i < kCampIDMax; ++i) {
    if (&matchups_data_[kMaxRoundID - 1][i] == d) {
      auto self_index = i;
      auto opponent_camp_index = (i % 2 == 0 ? i + 1 : i - 1);
      if (d->win) {
        return std::min(self_index, opponent_camp_index) + 1;
      } else {
        return std::max(self_index, opponent_camp_index) + 1;
      }
    }
  }
  CHECK(false) << "Cannot find proper matchups data";
  return 0;
}

void CampMatchups::ForeachMatchup(uint16_t battle_round, MatchupFunc func) {
  CHECK(battle_round != 0 && battle_round <= kMaxRoundID);
  CHECK(CurrentRound() >= battle_round);
  // CHECK(RoundFinished(battle_round));
  auto round_matchups_data = MatchupDataForRound(battle_round);
  for (auto i = 0; i < kCampIDMax - 1; i += 2) {
    func(&round_matchups_data[i], &round_matchups_data[i + 1]);
  }
}

void CampMatchups::UnfinishedBattle(uint16_t battle_round, MatchupFunc func) {
  CHECK(battle_round != 0 && battle_round <= kMaxRoundID);
  auto round_matchups_data = MatchupDataForRound(battle_round);
  for (auto i = 0; i < kCampIDMax - 1; i += 2) {
    auto& matchup_data = round_matchups_data[i];
    if (!matchup_data.set) {
      func(&matchup_data, &round_matchups_data[i + 1]);
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
  static_assert(kCampIDMax == 8, "Cannot handle camps other than 8 camps");
  if (current_battle_round_ == 0) {
    // Shuffle all camps
    alpha::Random::Shuffle(RoundMatchupsDataBegin(1), RoundMatchupsDataEnd(1));
  } else if (current_battle_round_ == 1) {
    // Camps are devided into 2 groups, in each group winners of last round
    // fight each other
    std::copy(RoundMatchupsDataBegin(1), RoundMatchupsDataEnd(1),
              RoundMatchupsDataBegin(2));
    auto begin = RoundMatchupsDataBegin(2);
    auto end = std::next(begin, 4);
    auto cmp = [](const MatchupData& d) { return d.win; };
    std::stable_partition(begin, end, cmp);
    std::stable_partition(end, RoundMatchupsDataEnd(2), cmp);
    std::for_each(RoundMatchupsDataBegin(2), RoundMatchupsDataEnd(2),
                  [](MatchupData& d) { d.ClearExceptCamp(); });
  } else if (current_battle_round_ == 2) {
    // Match between groups by game_log
    // ig:  win win <-> win win
    //      win lose <-> win lose
    //      lose win <-> lose win
    //      lose lose <-> lose lose
    using TupleType = std::tuple<std::string, int, CampID>;
    std::set<TupleType, std::greater<TupleType> > s;
    auto round_matchups_data = MatchupDataForRound(2);
    for (auto i = 0; i < kCampIDMax; ++i) {
      auto camp = round_matchups_data[i].camp;
      auto game_log = GetGameLog(camp, 2);
      s.insert(TupleType(game_log, kCampIDMax - i, camp));
    }
    std::transform(s.begin(), s.end(), RoundMatchupsDataBegin(3),
                   [](const TupleType& p) {
      MatchupData d;
      d.camp = std::get<2>(p);
      d.ClearExceptCamp();
      return d;
    });
  }

  ++current_battle_round_;
  return true;
}

MatchupData* CampMatchups::FindMatchupData(uint16_t battle_round, CampID camp) {
  CHECK(battle_round != 0 && battle_round <= kMaxRoundID)
      << "Invalid battle round: " << battle_round;
  auto index = battle_round - 1;
  auto it = std::find_if(
      std::begin(matchups_data_[index]), std::end(matchups_data_[index]),
      [&camp](const MatchupData& d) { return d.camp == camp; });
  CHECK(it != std::end(matchups_data_[index]));
  return &*it;
}

const MatchupData* CampMatchups::FindMatchupData(uint16_t battle_round,
                                                 CampID camp) const {
  return const_cast<CampMatchups*>(this)->FindMatchupData(battle_round, camp);
}

MatchupData* CampMatchups::RoundMatchupsDataBegin(uint16_t battle_round) {
  CHECK(battle_round != 0 && battle_round <= kMaxRoundID);
  return std::begin(matchups_data_[battle_round - 1]);
}

MatchupData* CampMatchups::RoundMatchupsDataEnd(uint16_t battle_round) {
  CHECK(battle_round != 0 && battle_round <= kMaxRoundID);
  return std::end(matchups_data_[battle_round - 1]);
}

const MatchupData* CampMatchups::RoundMatchupsDataBegin(uint16_t battle_round)
    const {
  CHECK(battle_round != 0 && battle_round <= kMaxRoundID);
  return std::begin(matchups_data_[battle_round - 1]);
}

const MatchupData* CampMatchups::RoundMatchupsDataEnd(uint16_t battle_round)
    const {
  CHECK(battle_round != 0 && battle_round <= kMaxRoundID);
  return std::end(matchups_data_[battle_round - 1]);
}

Camp::Camp(CampID id) : id_(id) {}

void Camp::ClearSeasonData() {
  chief_commander_ = 0;
  living_warriors_.clear();
  warriors_.clear();
}

void Camp::AddWarrior(UinType uin, bool dead) {
  auto p = warriors_.insert(uin);
  CHECK(p.second);
  if (!dead) {
    living_warriors_.insert(uin);
  }
}

void Camp::MarkWarriorDead(UinType uin) { living_warriors_.erase(uin); }

void Camp::ResetLivingWarriors() { living_warriors_ = warriors_; }

UinList Camp::RandomChooseLivingWarriors(size_t num) const {
  CHECK(LivingWarriorsNum() >= num);
  UinList choosen_warriors;
  auto choosen_warrior_iters = alpha::Random::Sample(
      living_warriors_.begin(), living_warriors_.end(), num);
  std::transform(choosen_warrior_iters.begin(), choosen_warrior_iters.end(),
                 std::back_inserter(choosen_warriors),
                 [](const UinSet::const_iterator& it) { return *it; });
  return choosen_warriors;
}

bool Camp::NoLivingWarriors() const { return living_warriors_.empty(); }

size_t Camp::LivingWarriorsNum() const { return living_warriors_.size(); }

size_t Camp::WarriorsNum() const { return warriors_.size(); }

bool operator<(const Camp& lhs, const Camp& rhs) { return lhs.id() < rhs.id(); }

Zone::Zone(uint16_t id, CampMatchups* matchups, CampLeaderList* leaders,
           LuckyWarriorList* lucky_warriors, GeneralInChiefList* generals)
    : id_(id),
      matchups_(matchups),
      leaders_(leaders),
      lucky_warriors_(lucky_warriors),
      generals_(generals) {
  for (auto i = 0u; i < kCampIDMax; ++i) {
    camps_.emplace_back((CampID)(i + 1));
  }
}

Camp* Zone::GetCamp(uint16_t camp_id) {
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
  d->initial_season = true;   //发布周的没有打的初始赛季
  d->season_finished = true;  //周四发布，所以算第一届打完了
  d->battle_round = 3;        // 同上
  d->battle_season = 1;       //同上
  for (auto i = 0; i < kMaxZoneNum; ++i) {
    d->matchups[i].Reset();
    d->camp_leaders[i].Clear();
    d->lucky_warriors[i].Clear();
    d->generals[i].Clear();
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
    auto index = zone_id - 1;
    auto matchups = &battle_data_saved_->matchups[index];
    auto leaders = &battle_data_saved_->camp_leaders[index];
    auto lucky_warriors = &battle_data_saved_->lucky_warriors[index];
    auto generals = &battle_data_saved_->generals[index];
    zones_.emplace_back(zone_id, matchups, leaders, lucky_warriors, generals);
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
  battle_data_saved_->season_finished = false;
  battle_data_saved_->last_season_data_dropped = false;
  battle_data_saved_->battle_round = 0;
  if (battle_data_saved_->initial_season) {
    battle_data_saved_->initial_season = false;
  } else {
    ++battle_data_saved_->battle_season;
  }
  ForeachZone([](Zone& zone) {
    for (auto i = 0; i < kCampIDMax; ++i) {
      auto camp = zone.GetCamp(i + 1);
      camp->ClearSeasonData();
    }
  });
  return true;
}

void BattleData::ResetMatchups() {
  for (auto i = 0u; i < kMaxZoneNum; ++i) {
    battle_data_saved_->matchups[i].Reset();
  }
}

void BattleData::ResetLeaders() {
  for (auto i = 0u; i < kMaxZoneNum; ++i) {
    battle_data_saved_->camp_leaders[i].Clear();
  }
}

void BattleData::ResetLuckyWarriors() {
  for (auto i = 0u; i < kMaxZoneNum; ++i) {
    battle_data_saved_->lucky_warriors[i].Clear();
  }
}

void BattleData::IncreaseCurrentRound() {
  CHECK(CurrentRound() <= kMaxRoundID);
  battle_data_saved_->battle_round++;
}

void BattleData::SetSeasonFinished() {
  battle_data_saved_->season_finished = true;
}

bool BattleData::InitialSeason() const {
  return battle_data_saved_->initial_season;
}

bool BattleData::SeasonFinished() const {
  return battle_data_saved_->season_finished;
}

bool BattleData::SeasonStarted() const {
  return battle_data_saved_->battle_round != 0;
}

bool BattleData::SeasonNotStarted() const { return !SeasonStarted(); }

uint16_t BattleData::CurrentRound() const {
  return battle_data_saved_->battle_round;
}

uint16_t BattleData::FinishedRound() const {
  auto current = CurrentRound();
  if (current == 0) {
    return 0;
  } else if (CurrentRoundFinished()) {
    return current;
  } else {
    return current - 1;
  }
}

bool BattleData::CurrentRoundFinished() const {
  auto current = CurrentRound();
  if (current == 0) {
    return true;
  }
  // matchups中的current round可能和BattleData中的current round不一样
  // 尤其是当你刚刚调完BattleData::SetCurrentRound
  return std::all_of(zones_.begin(), zones_.end(), [current](const Zone& zone) {
    return zone.matchups()->RoundFinished(current);
  });
}

WarriorLite WarriorLite::Create(UinType uin, uint16_t zone_id, CampID camp_id) {
  WarriorLite warrior_lite;
  warrior_lite.uin = uin;
  warrior_lite.zone_id = zone_id;
  warrior_lite.camp_id = camp_id;
  return warrior_lite;
}

Warrior Warrior::Create(UinType uin, uint16_t zone_id, CampID camp_id) {
  Warrior warrior;
  warrior.uin_ = uin;
  warrior.zone_id_ = zone_id;
  warrior.camp_id_ = camp_id;
  warrior.dead_ = false;
  warrior.last_draw_reward_round_ = 0;
  warrior.round_killing_num_ = 0;
  warrior.season_killing_num_ = 0;
  warrior.last_killed_warrior_ = 0;
  return warrior;
}

void Warrior::add_killing_num() {
  round_killing_num_ += 1;
  season_killing_num_ += 1;
}

void Warrior::ResetRoundData() {
  set_dead(false);
  set_last_killed_warrior(0);
  round_killing_num_ = 0;
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

Reward& Reward::Merge(const Reward& other) {
  CHECK(&other != this) << "Cannot merge self";
  battle_point += other.battle_point;
  for (auto i = 0u; i < other.next_goods_reward_index; ++i) {
    bool ok = AddGoods(other.goods_reward[i].id, other.goods_reward[i].num);
    CHECK(ok);
  }
  return *this;
}

bool Reward::AddGoods(uint32_t goods_id, uint32_t goods_num) {
  for (uint32_t i = 0; i < next_goods_reward_index; ++i) {
    if (goods_reward[i].id == goods_id) {
      goods_reward[i].num += goods_num;
      return true;
    }
  }
  if (next_goods_reward_index == kMaxGoodsRewardKind) {
    return false;
  }
  goods_reward[next_goods_reward_index].id = goods_id;
  goods_reward[next_goods_reward_index].num = goods_num;
  ++next_goods_reward_index;
  return true;
}

bool Reward::Empty() const {
  return next_goods_reward_index == 0 && battle_point == 0;
}

ZoneConf::ZoneConf(uint16_t id, unsigned max_camp_warriors_num,
                   unsigned max_level, const std::string& name)
    : zone_id_(id),
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

Reward ZoneConf::GetRoundReward(const std::string& game_log) const {
  auto it = round_reward_map_.find(game_log);
  CHECK(it != round_reward_map_.end())
      << "Cannot find round reward for game log: " << game_log;
  return it->second;
}
}
