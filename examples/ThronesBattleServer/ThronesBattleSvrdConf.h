/*
 * =============================================================================
 *
 *       Filename:  ThronesBattleSvrdConf.h
 *        Created:  12/17/15 17:11:29
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include <memory>
#include <vector>
#include <string>
#include <alpha/net_address.h>
#include "ThronesBattleSvrdDef.h"

namespace ThronesBattle {
class ServerConf final {
 public:
  static std::unique_ptr<ServerConf> Create(const char* file);
  uint32_t battle_data_file_size() const { return battle_data_file_size_; }
  uint32_t warriors_data_file_size() const { return warriors_data_file_size_; }
  uint32_t rewards_data_file_size() const { return rewards_data_file_size_; }
  std::string battle_data_file() const { return battle_data_file_; }
  std::string warriors_data_file() const { return warriors_data_file_; }
  std::string rewards_data_file() const { return rewards_data_file_; }

  bool InSignUpTime() const;
  bool InRewardTime(bool season_finished) const;
  time_t BattleRoundStartTime(uint16_t round) const;
  time_t NextDropLastSeasonDataTime(bool season_finished) const;
  time_t NextSeasonBaseTime() const;
  Reward lucky_warrior_reward() const { return lucky_warrior_reward_; }
  alpha::NetAddress fight_server_addr() const { return fight_server_addr_; }

  const ZoneConf* GetZoneConf(const Zone* zone);
  const ZoneConf* GetZoneConf(uint16_t zone_id);

 private:
  ServerConf() = default;
  bool InitFromFile(const char* file);
  time_t CurrentSeasonBaseTime() const;

  unsigned backup_interval_;
  unsigned signup_start_offset_;
  unsigned signup_finish_offset_;
  unsigned reward_time_finish_offset_;
  unsigned new_season_time_offset_;
  uint32_t battle_data_file_size_;
  uint32_t warriors_data_file_size_;
  uint32_t rewards_data_file_size_;
  alpha::NetAddress fight_server_addr_;
  alpha::NetAddress backup_server_addr_;
  Reward lucky_warrior_reward_;
  unsigned round_start_time_offsets_[kMaxRoundID];
  std::string battle_data_file_;
  std::string warriors_data_file_;
  std::string rewards_data_file_;
  std::vector<ZoneConf> zones_;
};
}
