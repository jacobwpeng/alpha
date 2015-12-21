/*
 * =============================================================================
 *
 *       Filename:  ThronesBattleSvrdConf.cc
 *        Created:  12/18/15 21:44:53
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "ThronesBattleSvrdConf.h"
#include <alpha/logger.h>
#include <alpha/compiler.h>
#include <alpha/logger.h>
#include <boost/foreach.hpp>
#include <boost/date_time.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace ThronesBattle {
namespace detail {
Reward ReadReward(const boost::property_tree::ptree& pt) {
  BOOST_AUTO(opt_battle_point, pt.get_optional<uint32_t>("<xmlattr>.point"));
  Reward reward = Reward::Create(opt_battle_point.get_value_or(0));
  BOOST_FOREACH(const boost::property_tree::ptree::value_type & child,
                pt.get_child("")) {
    if (child.first != "Goods") continue;
    uint32_t goods_id = child.second.get<uint32_t>("<xmlattr>.id");
    uint32_t goods_num = child.second.get<uint32_t>("<xmlattr>.num");
    bool ok = reward.AddGoods(goods_id, goods_num);
    CHECK(ok);
  }
  return reward;
}

void ReadFileOption(const boost::property_tree::ptree& pt, std::string* path,
                    uint32_t* size) {
  *path = pt.get<std::string>("<xmlattr>.file");
  *size = pt.get<uint32_t>("<xmlattr>.size") << 20;  // MiB
}
}

bool ServerConf::InSignUpTime() const {
  time_t now = time(NULL);
  time_t base = CurrentSeasonBaseTime();
  time_t start = base + signup_start_offset_;
  time_t finish = base + signup_finish_offset_;
  return now >= start && now < finish;
}

bool ServerConf::InRewardTime(bool season_finished) const {
  if (season_finished) {
    return true;
  } else {
    time_t now = time(NULL);
    time_t base = CurrentSeasonBaseTime();
    time_t finish = base + reward_time_finish_offset_;
    return now < finish;
  }
}

time_t ServerConf::BattleRoundStartTime(uint16_t round) const {
  CHECK(round != 0 && round <= kMaxRoundID);
  time_t base = CurrentSeasonBaseTime();
  return base + round_start_time_offsets_[round - 1];
}

time_t ServerConf::NextDropLastSeasonDataTime(bool season_finished) const {
  if (season_finished) {
    return NextSeasonBaseTime() + reward_time_finish_offset_;
  } else {
    return CurrentSeasonBaseTime() + reward_time_finish_offset_;
  }
}

time_t ServerConf::NextSeasonBaseTime() const {
  return CurrentSeasonBaseTime() + 7 * 24 * 3600;
}

time_t ServerConf::CurrentSeasonBaseTime() const {
  boost::gregorian::greg_weekday friday(boost::gregorian::Friday);
  boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
  boost::gregorian::date base_date =
      boost::gregorian::previous_weekday(now.date(), friday);
  boost::posix_time::ptime current_season_base_time(
      base_date, boost::posix_time::time_duration(14, 0, 0));
  if (now < current_season_base_time) {
    current_season_base_time -= boost::gregorian::days(7);
  }
  struct tm tm = boost::posix_time::to_tm(current_season_base_time);
  time_t t = mktime(&tm);
  CHECK(t != -1);
  DLOG_INFO << "Current season base time: " << t;
  return t;
}

std::unique_ptr<ServerConf> ServerConf::Create(const char* file) {
  std::unique_ptr<ServerConf> conf(new ServerConf);
  bool ok = conf->InitFromFile(file);
  return ok ? std::move(conf) : nullptr;
}

bool ServerConf::InitFromFile(const char* file) {
  CurrentSeasonBaseTime();
  using namespace boost::property_tree;
  ptree pt;
  try {
    read_xml(file, pt, boost::property_tree::xml_parser::no_comments);
    signup_start_offset_ = pt.get<unsigned>("Time.SignUp.<xmlattr>.start");
    signup_finish_offset_ = pt.get<unsigned>("Time.SignUp.<xmlattr>.finish");
    reward_time_finish_offset_ =
        pt.get<unsigned>("Time.RewardTime.<xmlattr>.finish");
    new_season_time_offset_ = pt.get<unsigned>("Time.NewSeason.<xmlattr>.time");
    BOOST_FOREACH(const ptree::value_type & child, pt.get_child("Time")) {
      if (child.first != "Fight") continue;
      uint16_t round = child.second.get<uint16_t>("<xmlattr>.round");
      unsigned t = child.second.get<unsigned>("<xmlattr>.start");
      CHECK(round != 0 && round <= kMaxRoundID);
      round_start_time_offsets_[round - 1] = t;
    }
    detail::ReadFileOption(pt.get_child("Server.BattleData"),
                           &battle_data_file_, &battle_data_file_size_);
    detail::ReadFileOption(pt.get_child("Server.WarriorsData"),
                           &warriors_data_file_, &warriors_data_file_size_);
    detail::ReadFileOption(pt.get_child("Server.RewardData"),
                           &rewards_data_file_, &rewards_data_file_size_);
    lucky_warrior_reward_ =
        detail::ReadReward(pt.get_child("LuckyWarriorReward"));
    BOOST_FOREACH(const ptree::value_type & child, pt.get_child("Zones")) {
      if (child.first != "Zone") continue;
      BOOST_AUTO(pt_zone, child.second);
      ZoneConf zone(pt_zone.get<uint16_t>("<xmlattr>.id"),
                    pt_zone.get<unsigned>("<xmlattr>.max_camp_warriors_num"),
                    pt_zone.get<unsigned>("<xmlattr>.max_level"),
                    pt_zone.get<std::string>("<xmlattr>.name"));
      BOOST_FOREACH(const ptree::value_type & grandchild,
                    pt_zone.get_child("Rewards")) {
        BOOST_AUTO(pt_reward, grandchild.second);
        if (grandchild.first == "RoundReward") {
          zone.AddRoundReward(pt_reward.get<std::string>("<xmlattr>.game_log"),
                              detail::ReadReward(pt_reward));
        } else if (grandchild.first == "RankReward") {
          zone.AddRankReward(pt_reward.get<unsigned>("<xmlattr>.max_rank"),
                             detail::ReadReward(pt_reward));

        } else {
          continue;
        }
      }
    }
    return true;
  }
  catch (ptree_error& e) {
    LOG_ERROR << "InitFromFile failed, file: " << file << ", " << e.what();
    return false;
  }
}
}
