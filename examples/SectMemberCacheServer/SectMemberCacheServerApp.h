/*
 * =============================================================================
 *
 *       Filename:  SectMemberCacheServerApp.h
 *        Created:  04/25/16 14:40:30
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include <ctime>
#include <map>
#include <set>
#include <alpha/slice.h>
#include <alpha/event_loop.h>
#include <alpha/UDPServer.h>
#include "proto/SectMemberCacheServer.pb.h"

struct UserInfoLite {
  unsigned uin;
  unsigned sect;
  unsigned level;
  time_t update_time;
};

bool operator<(const UserInfoLite& lhs, const UserInfoLite& rhs);

using UserInfoLiteSet = std::set<UserInfoLite>;
using SectMemberMap = std::map<unsigned, UserInfoLiteSet>;  // key: level
using SectMap = std::map<unsigned, SectMemberMap>;          // key: sect

struct UserIndex {
  UserInfoLiteSet::iterator it;
  UserInfoLiteSet* s;
};

using UserIndexMap = std::map<unsigned, UserIndex>;  // key: uin

class SectMemberCacheServerApp {
 public:
  SectMemberCacheServerApp(alpha::Slice ip, int port);
  int Run();

  void HandleUDPMessage(alpha::UDPSocket* socket, alpha::IOBuffer* buf,
                        size_t buf_len, const alpha::NetAddress& peer);
  int HandleReportSectMember(
      unsigned uin, const SectMemberCacheServerApi::ReportSectMember& r);
  int HandlePickMember(unsigned uin,
                       const SectMemberCacheServerApi::PickMemberRequest& req,
                       SectMemberCacheServerApi::PickMemberResponse* resp);

 private:
  static const size_t kMaxNumForOneLevel = 1024;
  const UserInfoLite* PickFromMemberMap(unsigned uin, unsigned init_level,
                                        const SectMemberMap& m);
  const UserInfoLite* PickFromMemberMapInLevel(unsigned uin, unsigned level,
                                               const SectMemberMap& m);
  const UserInfoLite* PickFromUserInfoSet(unsigned uin,
                                          const UserInfoLiteSet& v);
  UserIndex* RemoveUserFromSectMap(unsigned uin);
  std::string ip_;
  int port_;
  alpha::EventLoop loop_;
  alpha::UDPServer server_;
  SectMap sect_map_;
  UserIndexMap user_index_map_;
};
