/*
 * =============================================================================
 *
 *       Filename:  ThronesBattleSvrdDef.h
 *        Created:  12/15/15 14:35:44
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include <cstdint>
#include <set>
#include <map>
#include <vector>
#include <algorithm>
#include <functional>
#include <type_traits>

namespace ThronesBattle {

static const uint16_t kMinZoneID = 1;
static const uint16_t kMaxZoneID = 4;
static const uint16_t kCurrentZoneNum = 4;
static const uint16_t kMaxZoneNum = 10;
static const uint16_t kUnlimitedZoneID = 4;
static const uint16_t kMaxRoundID = 3;
static const uint16_t kMaxLuckyWarroirPerCamp = 5;
static_assert(kCurrentZoneNum == (kMaxZoneID - kMinZoneID + 1),
              "Invalid zone num");

enum Error : int32_t {
  kOk = 0,
  kServerInternalError = -10000,
  kMissingRequiredField = -10001,
  kInvalidArgument = -10002,
  kAllZoneAreFull = -10003,
  kNotInSignUpTime = -10004,
  kAlreadySignedUp = -10005,
  kNoSpaceForWarrior = -10006,
  kSeasonNotFinished = -10007,
  kNotCampLeader = -10008,
  kPickedLuckyWarriros = -10009,
  kNotInPickLuckyWarriorsTime = -10010,
  kNotSignedUp = -10011,
  kNotLuckyWarrior = -10012,
  kNoRoundReward = -10013,
  kNoLastSeasonWarrior = -10014,
  kRoundNotStarted = -10015,
};

enum CampID : uint16_t {
  kMin = 1,
  kDongZhuo = 1,
  kYuanShao = 2,
  kZhangJiao = 3,
  kCaoCao = 4,
  kLiuBei = 5,
  kSunCe = 6,
  kMaChao = 7,
  kLvBu = 8,
  kMax = 8
};
static const uint16_t kCampIDMax = static_cast<uint16_t>(CampID::kMax);
bool ValidCampID(uint16_t camp);
bool ValidZoneID(uint16_t zone_id);

using UinType = uint32_t;
using UinList = std::vector<UinType>;
using UinSet = std::set<UinType>;
struct General {
  CampID camp;
  UinType uin;
  uint32_t season;
};

class GeneralInChiefList {
 public:
  void AddGeneralInChief(CampID camp, UinType uin, uint32_t season);
  std::vector<General> Get(unsigned start, unsigned num) const;
  void Clear();
  uint16_t Size() const { return next_index_; }

 private:
  static const unsigned kMaxGeneralNum = 1000;
  uint16_t next_index_;
  General generals_[kMaxGeneralNum];
};

class LuckyWarriorList {
 public:
  void AddLuckyWarrior(CampID camp, UinType uin);
  std::vector<UinType> Get(uint16_t camp_id) const;
  bool HasWarrior(UinType uin) const;
  bool PickedLuckyWarriors(CampID camp_id) const;
  void Clear();

 private:
  UinType lucky_warriors_[kCampIDMax][kMaxLuckyWarroirPerCamp];
};

struct CampLeader {
  bool empty() const { return uin == 0; }
  uint8_t picked_lucky_warriors;
  UinType uin;
  uint32_t killing_num;
};

class CampLeaderList {
 public:
  CampLeader GetLeader(CampID camp_id);
  void SetPickedLuckyWarriors(CampID camp_id);
  void Clear();
  void Notify(CampID camp_id, UinType uin, uint32_t killing_num);

 private:
  CampLeader* GetLeaderPtr(CampID camp_id);
  CampLeader leaders_[kCampIDMax];
};

struct MatchupData {
  void Clear();
  void ClearExceptCamp();
  void UpdateBattleInfo(bool win, uint32_t final_living_warriors_num);
  uint8_t win;
  uint8_t set;
  CampID camp;
  uint32_t final_living_warriors_num;
};

class CampMatchups {
 public:
  using MatchupFunc =
      std::function<void(const MatchupData*, const MatchupData*)>;
  bool BattleNotStarted() const;
  bool CurrentRoundFinished() const;
  bool RoundFinished(uint16_t battle_round) const;
  uint16_t CurrentRound() const;
  uint16_t FinishedRound() const;
  std::string GetGameLog(CampID camp, uint16_t max_battle_round) const;
  uint32_t GetFinalLivingWarriorsNum(uint16_t battle_round, CampID camp) const;
  // 如果最后一轮的次序确定，则返回对应次序，否则返回0
  uint16_t FinalRank(CampID camp) const;

  MatchupData* MatchupDataForRound(uint16_t battle_round);
  void Reset();
  void SetBattleResult(CampID camp, bool win,
                       uint32_t final_living_warriors_num);
  void ForeachMatchup(uint16_t battle_round, MatchupFunc func);
  void UnfinishedBattle(uint16_t battle_round, MatchupFunc func);
  bool StartNextRound();

  MatchupData* RoundMatchupsDataBegin(uint16_t battle_round);
  MatchupData* RoundMatchupsDataEnd(uint16_t battle_round);
  const MatchupData* RoundMatchupsDataBegin(uint16_t battle_round) const;
  const MatchupData* RoundMatchupsDataEnd(uint16_t battle_round) const;

 private:
  MatchupData* FindMatchupData(uint16_t battle_round, CampID camp);
  const MatchupData* FindMatchupData(uint16_t battle_round, CampID camp) const;
  uint16_t current_battle_round_;
  MatchupData matchups_data_[kMaxRoundID][kCampIDMax];
};

class Camp final {
 public:
  Camp(CampID id);
  CampID id() const { return id_; }
  void ClearSeasonData();
  void AddWarrior(UinType uin, bool dead = false);
  void MarkWarriorDead(UinType uin);
  void ResetLivingWarriors();

  const UinSet& living_warriors() const { return living_warriors_; }
  bool NoLivingWarriors() const;
  size_t LivingWarriorsNum() const;
  size_t WarriorsNum() const;
  const UinSet& Warriors() const { return warriors_; }
  template <typename LAMBDA>
  void ForeachWarrior(LAMBDA lambda) const {
    std::for_each(warriors_.begin(), warriors_.end(), lambda);
  }

 private:
  CampID id_;
  UinType chief_commander_;
  UinSet living_warriors_;
  UinSet warriors_;
};

bool operator<(const Camp& lhs, const Camp& rhs);

class Zone final {
 public:
  Zone(uint16_t id, CampMatchups* matchups, CampLeaderList* leaders,
       LuckyWarriorList* lucky_warriors, GeneralInChiefList* generals);
  uint16_t id() const { return id_; }
  Camp* GetCamp(uint16_t camp_id);
  CampMatchups* matchups() { return matchups_; }
  const CampMatchups* matchups() const { return matchups_; }
  CampLeaderList* leaders() { return leaders_; }
  const CampLeaderList* leaders() const { return leaders_; }
  LuckyWarriorList* lucky_warriors() { return lucky_warriors_; }
  const LuckyWarriorList* lucky_warriors() const { return lucky_warriors_; }
  GeneralInChiefList* generals() { return generals_; }
  const GeneralInChiefList* generals() const { return generals_; }

 private:
  uint16_t id_;
  CampMatchups* matchups_;
  CampLeaderList* leaders_;
  LuckyWarriorList* lucky_warriors_;
  GeneralInChiefList* generals_;
  std::vector<Camp> camps_;
};

struct BattleDataSaved {
  static BattleDataSaved* Create(void* p, size_t sz);
  static BattleDataSaved* Restore(void* p, size_t sz);

  static const uint32_t kMagic = 0xf8219e4d;
  uint8_t last_season_data_dropped;
  uint8_t season_finished;
  uint8_t initial_season;
  uint16_t battle_round;
  uint32_t battle_season;
  uint32_t magic;
  GeneralInChiefList generals[kMaxZoneNum];
  LuckyWarriorList lucky_warriors[kMaxZoneNum];
  CampLeaderList camp_leaders[kMaxZoneNum];
  CampMatchups matchups[kMaxZoneNum];
};

class BattleData final {
 public:
  BattleData(BattleDataSaved* d);
  bool last_season_data_dropped() const;
  void set_last_season_data_dropped(bool dropped);
  Zone* GetZone(uint16_t zone_id);
  template <typename LAMBDA>
  void ForeachZone(LAMBDA lambda) {
    std::for_each(std::begin(zones_), std::end(zones_), lambda);
  };

  template <typename LAMBDA>
  void ForeachZone(LAMBDA lambda) const {
    std::for_each(std::begin(zones_), std::end(zones_), lambda);
  };

  bool ChangeSeason();
  void ResetMatchups();
  void ResetLeaders();
  void ResetLuckyWarriors();
  void SetCurrentRound(uint16_t battle_round);
  void SetSeasonFinished();

  bool InitialSeason() const;
  bool SeasonFinished() const;
  bool SeasonStarted() const;
  bool SeasonNotStarted() const;
  uint16_t CurrentRound() const;
  uint16_t FinishedRound() const;
  uint32_t CurrentSeason() const { return battle_data_saved_->battle_season; }
  bool CurrentRoundFinished() const;

 private:
  BattleDataSaved* battle_data_saved_;
  std::vector<Zone> zones_;
};

struct Goods {
  uint32_t id;
  uint32_t num;
};

class Warrior final {
 public:
  static Warrior Create(UinType uin, uint16_t zone_id, CampID camp_id);
  UinType uin() const { return uin_; }
  bool dead() const { return dead_; }
  uint16_t zone_id() const { return zone_id_; }
  CampID camp_id() const { return camp_id_; }
  uint32_t round_killing_num() const { return round_killing_num_; }
  uint32_t season_killing_num() const { return season_killing_num_; }
  UinType last_killed_warrior() const { return last_killed_warrior_; }

  void set_dead(bool dead) { dead_ = dead; }
  void add_killing_num();
  void set_last_killed_warrior(UinType uin) { last_killed_warrior_ = uin; }
  void ResetRoundData();

 private:
  uint8_t dead_;
  uint16_t zone_id_;
  CampID camp_id_;
  uint16_t last_draw_reward_round_;
  uint32_t round_killing_num_;
  uint32_t season_killing_num_;
  UinType last_killed_warrior_;
  UinType uin_;
};

struct WarriorLite final {
  static WarriorLite Create(UinType uin, uint16_t zone_id, CampID camp_id);
  uint16_t zone_id;
  CampID camp_id;
  UinType uin;
};

// TODO: 改成一个class
struct Reward final {
  static const int kMaxGoodsRewardKind = 5;
  static Reward Create(uint32_t battle_point);

  void Clear();
  Reward& Merge(const Reward& other);
  bool AddGoods(uint32_t goods_id, uint32_t goods_num);
  bool Empty() const;
  int goods_size() const { return next_goods_reward_index; }

  uint16_t next_goods_reward_index;
  uint32_t battle_point;
  Goods goods_reward[kMaxGoodsRewardKind];
};

class ZoneConf final {
 public:
  ZoneConf(uint16_t zone_id, unsigned max_camp_warriors_num, unsigned max_level,
           const std::string& name);

  uint16_t zone_id() const { return zone_id_; }
  unsigned max_camp_warriors_num() const { return max_camp_warriors_num_; }
  unsigned max_level() const { return max_level_; }
  void AddRoundReward(const std::string& game_log, const Reward& reward);
  void AddRankReward(unsigned max_rank, const Reward& reward);

  Reward GetRoundReward(const std::string& game_log) const;

 private:
  uint16_t zone_id_;
  unsigned max_camp_warriors_num_;
  unsigned max_level_;
  std::string name_;
  std::map<unsigned, Reward> rank_reward_map_;
  std::map<std::string, Reward> round_reward_map_;
};

static_assert(std::is_pod<BattleDataSaved>::value,
              "BattleDataSaved should be POD type");
static_assert(std::is_pod<Warrior>::value, "Warriro should be POD type");
};
