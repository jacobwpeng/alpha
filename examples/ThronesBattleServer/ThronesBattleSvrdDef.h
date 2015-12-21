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
static const uint16_t kMaxRoundID = 3;

static_assert(kCurrentZoneNum == (kMaxZoneID - kMinZoneID + 1),
              "Invalid zone num");
enum class CampID : uint16_t {
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
bool ValidCampID(CampID camp);
bool ValidZoneID(uint16_t zone_id);

using UinType = uint32_t;
struct General {
  UinType uin;
  CampID camp;
};

class GeneralInChiefList {
 public:
  void AddGeneralInChief(UinType uin, CampID camp);
  std::vector<General> Get(unsigned zone, unsigned start, unsigned num) const;

 private:
  static const unsigned kMaxGeneralNum = 1000;
  uint16_t next_index_;
  General generals[kMaxZoneNum][kMaxGeneralNum];
};

class LuckyWarriorList {
 public:
  void AddLuckyWarrior(unsigned zone, CampID camp, UinType uin);
  std::vector<UinType> Get(unsigned zone, unsigned camp) const;
  void Clear();

 private:
  static const uint16_t kMaxLuckyWarroirPerCamp = 5;
  UinType lucky_warriros_[kMaxZoneNum][kCampIDMax][kMaxLuckyWarroirPerCamp];
};

class CampMatchups {
 public:
  using MatchupFunc = std::function<void(CampID, CampID)>;
  bool BattleNotStarted() const;
  bool CurrentRoundFinished() const;
  bool RoundFinished(uint16_t battle_round) const;
  uint16_t CurrentRound() const;
  std::string GetGameLog(CampID camp, uint16_t max_battle_round) const;

  void Reset();
  void SetBattleResult(CampID camp, bool win, uint32_t final_live_warriors_num);
  void ForeachMatchup(uint16_t battle_round, MatchupFunc func);
  void UnfinishedBattle(uint16_t battle_round, MatchupFunc func);
  bool StartNextRound();

 private:
  struct MatchupData {
    bool win;
    bool set;
    CampID camp;
    uint32_t final_live_warriors_num;
  };
  MatchupData* FindMatchupData(uint16_t battle_round, CampID camp);
  const MatchupData* FindMatchupData(uint16_t battle_round, CampID camp) const;
  uint16_t current_battle_round_;
  MatchupData matchups_data_[kMaxRoundID][kCampIDMax];
};

class Camp {
 public:
  Camp(CampID id);
  CampID id() const { return id_; }
  void AddWarrior(UinType uin, bool dead = false);
  void SetWarriorDead(UinType uin);
  std::vector<UinType> LivingWarriors() const;

 private:
  CampID id_;
  UinType chief_commander_;
  std::set<UinType> live_warriors_;
  std::set<UinType> warriors_;
};

bool operator<(const Camp& lhs, const Camp& rhs);

class Zone final {
 public:
  Zone(uint16_t id, CampMatchups* matchups, CampMatchups* last_season_matchups);
  uint16_t id() const { return id_; }
  Camp* GetCamp(CampID camp_id);
  CampMatchups* matchups() { return matchups_; }
  CampMatchups* last_season_matchups() { return last_season_matchups_; }
  const CampMatchups* matchups() const { return matchups_; }
  const CampMatchups* last_season_matchups() const {
    return last_season_matchups_;
  }

 private:
  uint16_t id_;
  CampMatchups* matchups_;
  CampMatchups* last_season_matchups_;
  std::vector<Camp> camps_;
};

struct BattleDataSaved {
  static BattleDataSaved* Create(void* p, size_t sz);
  static BattleDataSaved* Restore(void* p, size_t sz);

  static const uint32_t kMagic = 0xf8219e4d;
  bool last_season_data_dropped;
  bool season_finished_;
  uint16_t battle_round;
  uint32_t last_battle_season;
  uint32_t battle_season;
  uint32_t magic;
  GeneralInChiefList General_in_chief_list;
  LuckyWarriorList lucky_warriors;
  CampMatchups last_season_matchups[kMaxZoneNum];
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
  void DropLastSeasonData();
  void SetCurrentRound(uint16_t battle_round);
  void SetSeasonFinished();

  bool SeasonFinished() const;
  bool SeasonNotStarted() const;
  uint16_t CurrentRound() const;
  uint32_t LastSeason() const { return battle_data_saved_->last_battle_season; }
  uint32_t CurrentSeason() const { return battle_data_saved_->battle_season; }
  bool CurrentRoundFinished() const;

 private:
  BattleDataSaved* battle_data_saved_;
  std::vector<Zone> zones_;
};

struct Goods {
  uint32_t goods_id;
  uint32_t goods_num;
};

struct Reward {
  static const int kMaxGoodsRewardKind = 5;
  static Reward Create(uint32_t battle_point);

  void Clear();
  bool AddGoods(uint32_t goods_id, uint32_t goods_num);
  bool Empty() const;

  uint16_t next_goods_reward_index;
  uint32_t battle_point;
  Goods goods_reward[kMaxGoodsRewardKind];
};

class Warrior {
 public:
  UinType uin() const { return uin_; }
  bool dead() const { return dead_; }
  uint16_t zone_id() const { return zone_id_; }
  CampID camp_id() const { return camp_id_; }

 private:
  bool dead_;
  uint16_t zone_id_;
  CampID camp_id_;
  uint16_t last_draw_reward_round;
  uint32_t killed_;
  UinType uin_;
};

class ZoneConf {
 public:
  ZoneConf(uint16_t id, unsigned max_camp_warriors_num, unsigned max_level,
           const std::string& name);

  void AddRoundReward(const std::string& game_log, const Reward& reward);
  void AddRankReward(unsigned max_rank, const Reward& reward);

 private:
  uint16_t id_;
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
