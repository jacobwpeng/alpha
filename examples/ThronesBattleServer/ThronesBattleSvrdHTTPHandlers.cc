/*
 * =============================================================================
 *
 *       Filename:  ThronesBattleSvrdHTTPHandlers.cc
 *        Created:  01/09/16 10:07:46
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "ThronesBattleSvrdApp.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <alpha/HTTPResponseBuilder.h>

namespace ThronesBattle {
void ServerApp::HandleHTTPMessage(alpha::TcpConnectionPtr conn,
                                  const alpha::HTTPMessage& message) {
  auto method = message.Method();
  if (method != "GET") {
    alpha::HTTPResponseBuilder builder(conn);
    builder.status(400, "Bad Request").SendWithEOM();
    return;
  }
  auto path = message.Path();
  if (path == "/status") {
    boost::property_tree::ptree pt;
    pt.put("CurrentSeason", battle_data_->CurrentSeason());
    pt.put("InitialSeason", battle_data_->InitialSeason());
    pt.put("CurrentRound", battle_data_->CurrentRound());
    pt.put("CurrentRoundFinished", battle_data_->CurrentRoundFinished());
    pt.put("FinishedRound", battle_data_->FinishedRound());
    pt.put("SeasonStarted", battle_data_->SeasonStarted());
    pt.put("SeasonFinished", battle_data_->SeasonFinished());

    boost::property_tree::ptree pt_zones;
    auto fill_zone_info = [&pt_zones](Zone& zone) {
      boost::property_tree::ptree pt_zone;
      boost::property_tree::ptree pt_camps;
      pt_zone.put("zone", zone.id());
      for (int i = 0; i < kCampIDMax; ++i) {
        auto camp_id = (CampID)(i + 1);
        auto camp = zone.GetCamp(camp_id);
        boost::property_tree::ptree pt_camp;
        pt_camp.put("camp", camp->id());
        pt_camp.put("LivingWarriorsNum", camp->LivingWarriorsNum());
        pt_camp.put("WarriorsNum", camp->WarriorsNum());
        pt_camps.push_back(std::make_pair("", pt_camp));
      }
      pt_zone.add_child("camps", pt_camps);

      boost::property_tree::ptree pt_matchups;
      auto matchups = zone.matchups();
      pt_matchups.put("BattleNotStarted", matchups->BattleNotStarted());
      pt_matchups.put("CurrentRound", matchups->CurrentRound());
      pt_matchups.put("CurrentRoundFinished", matchups->CurrentRoundFinished());
      boost::property_tree::ptree pt_matchup_pairs;
      for (int i = 1; i <= matchups->CurrentRound(); ++i) {
        boost::property_tree::ptree p;
        auto fill_matchup_pair = [&p](const MatchupData* one,
                                      const MatchupData* the_other) {
          auto matchup_data_to_ptree = [](const MatchupData* d) {
            boost::property_tree::ptree pt;
            pt.put("set", d->set);
            pt.put("camp", d->camp);
            pt.put("win", d->win);
            pt.put("final_living_warriors_num", d->final_living_warriors_num);
            return pt;
          };
          p.push_back(std::make_pair("", matchup_data_to_ptree(one)));
          p.push_back(std::make_pair("", matchup_data_to_ptree(the_other)));
        };
        matchups->ForeachMatchup(i, fill_matchup_pair);
        pt_matchup_pairs.push_back(std::make_pair("", p));
      }
      pt_matchups.add_child("matchup_pairs", pt_matchup_pairs);
      pt_zone.add_child("matchups", pt_matchups);

      boost::property_tree::ptree pt_leaders;
      auto leaders = zone.leaders();
      for (int i = CampID::kMin; i <= CampID::kMax; ++i) {
        boost::property_tree::ptree pt_leader;
        auto camp_id = (CampID)i;
        auto leader = leaders->GetLeader(camp_id);
        pt_leader.put("camp", camp_id);
        pt_leader.put("uin", leader.uin);
        pt_leader.put("picked_lucky_warriors",
                      (bool)leader.picked_lucky_warriors);
        pt_leader.put("killing_num", leader.killing_num);
        pt_leaders.push_back(std::make_pair("", pt_leader));
      }
      pt_zone.add_child("leaders", pt_leaders);

      boost::property_tree::ptree pt_lucky_warriors;
      auto lucky_warriors = zone.lucky_warriors();
      for (int i = CampID::kMin; i <= CampID::kMax; ++i) {
        boost::property_tree::ptree pt_camp_lucky_warriors_info;
        boost::property_tree::ptree pt_camp_lucky_warriors;
        auto warriors = lucky_warriors->Get(i);
        for (const auto& uin : warriors) {
          boost::property_tree::ptree pt;
          pt.put("", uin);
          pt_camp_lucky_warriors.push_back(std::make_pair("", pt));
        }
        pt_camp_lucky_warriors_info.put("camp", i);
        pt_camp_lucky_warriors_info.add_child("warriors",
                                              pt_camp_lucky_warriors);
        pt_lucky_warriors.push_back(
            std::make_pair("", pt_camp_lucky_warriors_info));
      }
      pt_zone.add_child("lucky_warriors", pt_lucky_warriors);

      boost::property_tree::ptree pt_generals;
      auto generals = zone.generals()->Get(0, zone.generals()->Size());
      for (const auto& general : generals) {
        boost::property_tree::ptree pt_general;
        pt_general.put("camp", general.camp);
        pt_general.put("uin", general.uin);
        pt_general.put("season", general.season);
        pt_generals.push_back(std::make_pair("", pt_general));
      }
      pt_zone.add_child("generals", pt_generals);
      pt_zones.push_back(std::make_pair("", pt_zone));
    };
    battle_data_->ForeachZone(fill_zone_info);
    pt.add_child("zones", pt_zones);
    std::ostringstream oss;
    boost::property_tree::write_json(oss, pt);

    alpha::HTTPResponseBuilder builder(conn);
    builder.status(200, "OK").body(oss.str()).SendWithEOM();
  } else if (path == "/warrior") {
    UinType uin = 0;
    auto params = message.Params();
    auto it = params.find("uin");
    if (it != params.end()) {
      try {
        uin = std::stoul(it->second);
      }
      catch (std::exception& e) {
        LOG_INFO << "Invalid uin: " << it->second;
      }
    }
    std::string reply;
    if (uin) {
      boost::property_tree::ptree pt;
      auto it = warriors_->find(uin);
      if (it != warriors_->end()) {
        auto& warrior = it->second;
        pt.put("current_season", true);
        pt.put("uin", warrior.uin());
        pt.put("zone", warrior.zone_id());
        pt.put("camp", warrior.camp_id());
        pt.put("dead", warrior.dead());
        pt.put("last_killed_warrior", warrior.last_killed_warrior());
        pt.put("round_killing_num", warrior.round_killing_num());
        pt.put("season_killing_num", warrior.season_killing_num());
        std::ostringstream oss;
        boost::property_tree::write_json(oss, pt);
        reply = oss.str();
      } else {
        auto it = rewards_->find(uin);
        if (it != rewards_->end()) {
          auto& lite = it->second;
          pt.put("current_season", false);
          pt.put("uin", lite.uin);
          pt.put("zone", lite.zone_id);
          pt.put("camp", lite.camp_id);
          std::ostringstream oss;
          boost::property_tree::write_json(oss, pt);
          reply = oss.str();
        }
      }
    }
    alpha::HTTPResponseBuilder builder(conn);
    if (reply.empty()) {
      builder.status(400, "Bad Request").SendWithEOM();
    } else {
      builder.status(200, "OK").body(reply).SendWithEOM();
    }
  }
}
}
