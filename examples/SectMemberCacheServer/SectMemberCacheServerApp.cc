/*
 * =============================================================================
 *
 *       Filename:  SectMemberCacheServerApp.cc
 *        Created:  04/25/16 14:42:01
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "SectMemberCacheServerApp.h"
#include <alpha/logger.h>
#include <alpha/random.h>

static const int kServerUnknownSect = -10001;
static const int kServerNoMatchedMember = -10002;

using namespace SectMemberCacheServerApi;

bool operator<(const UserInfoLite& lhs, const UserInfoLite& rhs) {
  if (lhs.update_time != rhs.update_time) {
    return lhs.update_time < rhs.update_time;
  }
  if (lhs.uin != rhs.uin) {
    return lhs.uin < rhs.uin;
  }
  if (lhs.sect != rhs.sect) {
    return lhs.sect < rhs.sect;
  }
  if (lhs.level != rhs.level) {
    return lhs.level < rhs.level;
  }
  return false;
}

SectMemberCacheServerApp::SectMemberCacheServerApp(alpha::Slice ip, int port)
    : ip_(ip.ToString()), port_(port), server_(&loop_) {}

int SectMemberCacheServerApp::Run() {
  using namespace std::placeholders;
  server_.SetMessageCallback(std::bind(
      &SectMemberCacheServerApp::HandleUDPMessage, this, _1, _2, _3, _4));
  alpha::NetAddress addr(ip_, port_);
  LOG_INFO << "Running at: " << addr;
  if (!server_.Run(addr)) {
    LOG_ERROR << "Run server failed";
    return EXIT_FAILURE;
  }
  loop_.Run();
  return 0;
}

void SectMemberCacheServerApp::HandleUDPMessage(alpha::UDPSocket* socket,
                                                alpha::IOBuffer* buf,
                                                size_t buf_len,
                                                const alpha::NetAddress& peer) {
  DLOG_INFO << "Receive UDP message, size: " << buf_len;
  SectMemberCacheServerApi::Message m;
  bool ok = m.ParseFromArray(buf->data(), buf_len);
  if (!ok) {
    LOG_WARNING << "Parse Message failed, buf_len: " << buf_len;
    return;
  }
  SectMemberCacheServerApi::Message reply = m;
  reply.clear_payload();
  bool need_reply = false;
  if (m.cmd() == REPORT) {
    ReportSectMember req;
    ok = req.ParseFromString(m.payload());
    if (!ok) {
      LOG_WARNING << "Parse ReportSectMember failed";
      return;
    }
    HandleReportSectMember(m.uin(), req);
  } else if (m.cmd() == PICK_MEMBER) {
    PickMemberRequest req;
    PickMemberResponse resp;
    ok = req.ParseFromString(m.payload());
    if (!ok) {
      LOG_WARNING << "Parse PickMemberRequest failed";
      return;
    }
    int err = HandlePickMember(m.uin(), req, &resp);
    reply.set_err(err);
    ok = resp.SerializeToString(reply.mutable_payload());
    CHECK(ok);
    need_reply = true;
  }

  if (!need_reply) {
    return;
  }

  alpha::IOBufferWithSize out(reply.ByteSize());
  ok = reply.SerializeToArray(out.data(), out.size());
  int rc = socket->SendTo(&out, out.size(), peer);
  if (rc < 0) {
    PLOG_WARNING << "SendTo failed, rc: " << rc;
  } else {
    DLOG_INFO << "Send udp message to " << peer << ", size: " << out.size();
  }
}

int SectMemberCacheServerApp::HandleReportSectMember(
    unsigned uin, const ReportSectMember& r) {
  DLOG_INFO << "Report sect member, uin: " << uin << ", sect: " << r.sect()
            << ", level: " << r.level();
  auto user_index = RemoveUserFromSectMap(uin);
  auto it = sect_map_.find(r.sect());
  if (it == sect_map_.end()) {
    LOG_INFO << "Create map for sect: " << r.sect();
  }
  auto sect_members = &sect_map_[r.sect()];
  auto level_it = sect_members->find(r.level());
  if (level_it == sect_members->end()) {
    LOG_INFO << "Create map for level: " << r.level();
  }
  auto one_level_members = &(*sect_members)[r.level()];
  // 最多保存kMaxNumForOneLevel个人
  if (one_level_members->size() >= kMaxNumForOneLevel) {
    // 由于UserInfoLite优先根据update_time排序,
    // 所以第一个肯定是时间戳最早的那个
    auto expired_uin = one_level_members->begin()->uin;
    RemoveUserFromSectMap(expired_uin);
    user_index_map_.erase(expired_uin);
  }
  UserInfoLite userinfo = {.uin = uin,
                           .sect = r.sect(),
                           .level = r.level(),
                           .update_time = time(NULL)};
  auto p = one_level_members->insert(userinfo);
  if (user_index == nullptr) {
    user_index = &user_index_map_[uin];
  }
  user_index->s = one_level_members;
  user_index->it = p.first;
  return 0;
}

int SectMemberCacheServerApp::HandlePickMember(unsigned uin,
                                               const PickMemberRequest& req,
                                               PickMemberResponse* resp) {
  DLOG_INFO << "uin: " << uin << ", sect: " << req.sect()
            << ", level: " << req.user_level();
  auto it = sect_map_.find(req.sect());
  if (it == sect_map_.end()) {
    return kServerNoMatchedMember;
  }
  auto userinfo = PickFromMemberMap(uin, req.user_level(), it->second);
  if (userinfo == NULL) {
    return kServerNoMatchedMember;
  }
  resp->set_member_uin(userinfo->uin);
  resp->set_member_level(userinfo->level);
  resp->set_member_update_time(userinfo->update_time);
  return 0;
}

const UserInfoLite* SectMemberCacheServerApp::PickFromMemberMap(
    unsigned uin, unsigned init_level, const SectMemberMap& m) {
  CHECK(init_level > 0);
  if (m.empty()) {
    return nullptr;
  }
  if (m.size() == 1) {
    return PickFromUserInfoSet(uin, m.begin()->second);
  }
  // 现在玩家所在的等级段进入查找
  const UserInfoLite* userinfo = PickFromMemberMapInLevel(uin, init_level, m);
  if (userinfo) {
    return userinfo;
  }

  // 再到临近等级段逐一查找
  // 时间复杂度看起来比较高
  // 实际上因为每个Set大小都不大,
  // 并且外网环境基本肯定会在所在等级段找到对应玩家
  // 所以应该不会是性能瓶颈
  const int level_begin = m.begin()->first;
  const int level_end = m.rbegin()->first;
  int level_search_down = init_level - 1;
  int level_search_up = init_level + 1;
  do {
    // 优先等级高的方向搜索
    if (level_search_up <= level_end) {
      userinfo = PickFromMemberMapInLevel(uin, level_search_up, m);
      if (userinfo) {
        break;
      }
    }
    if (level_search_down >= level_begin) {
      userinfo = PickFromMemberMapInLevel(uin, level_search_down, m);
      if (userinfo) {
        break;
      }
    }
    --level_search_down;
    ++level_search_up;
    if (level_search_down < level_begin && level_search_up > level_end) {
      // 已经超过了可搜索范围
      break;
    }
  } while (1);
  return userinfo;
}

UserIndex* SectMemberCacheServerApp::RemoveUserFromSectMap(unsigned uin) {
  auto it = user_index_map_.find(uin);
  if (it == user_index_map_.end()) {
    return nullptr;
  }
  auto* user_index = &it->second;
  user_index->s->erase(user_index->it);
  return user_index;
}

const UserInfoLite* SectMemberCacheServerApp::PickFromMemberMapInLevel(
    unsigned uin, unsigned level, const SectMemberMap& m) {
  auto it = m.find(level);
  if (it == m.end()) {
    return nullptr;
  }
  return PickFromUserInfoSet(uin, it->second);
}

const UserInfoLite* SectMemberCacheServerApp::PickFromUserInfoSet(
    unsigned uin, const UserInfoLiteSet& v) {
  if (v.empty()) {
    return nullptr;
  }
  if (v.size() == 1 && v.begin()->uin == uin) {
    return nullptr;
  }
  // 经过上面两重判断后保证一定能随机到玩家
  auto r = alpha::Random::Rand32(1, v.size());
  // O(logN), 如有性能瓶颈可以优化
  auto it = std::next(v.begin(), r);
  if (it->uin == uin) {
    // 恰好随机到自己, 返回第一个玩家
    it = v.begin();
  }
  return &*it;
}
