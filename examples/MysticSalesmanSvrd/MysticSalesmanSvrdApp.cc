/*
 * =============================================================================
 *
 *       Filename:  MysticSalesmanSvrdApp.cc
 *        Created:  12/18/15 15:02:43
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "MysticSalesmanSvrdApp.h"
#include <unistd.h>
#include <map>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <alpha/logger.h>
#include <alpha/format.h>
#include <alpha/HTTPResponseBuilder.h>
#include "proto/MysticSalesmanSvrd.pb.h"

MysticSalesmanSvrdApp::MysticSalesmanSvrdApp(int udp_port, int http_port,
                                             const char* mmap_file_path)
    : udp_port_(udp_port),
      http_port_(http_port),
      mmap_file_path_(mmap_file_path),
      udp_server_(&loop_),
      http_server_(&loop_) {}

int MysticSalesmanSvrdApp::Run() {
  using namespace std::placeholders;
  auto err = Daemonize();
  if (err) {
    PLOG_ERROR << "Daemonize failed, err: " << err;
    return err;
  }

  auto flags = alpha::MMapFile::kCreateIfNotExists;
  mmap_file_ =
      alpha::MMapFile::Open(mmap_file_path_.c_str(), kMMapFileSize, flags);
  if (mmap_file_ == nullptr) {
    LOG_ERROR << "Open mmap file failed, path: " << mmap_file_path_.c_str();
    return EXIT_FAILURE;
  }

  auto start = reinterpret_cast<char*>(mmap_file_->start());
  if (mmap_file_->newly_created()) {
    user_group_map_ = UserGroupMap::Create(start, mmap_file_->size());
  } else {
    user_group_map_ = UserGroupMap::Restore(start, mmap_file_->size());
  }

  if (user_group_map_ == nullptr) {
    const char* op = mmap_file_->newly_created() ? "Create" : "Restore";
    LOG_ERROR << op << " user group map from mmap file failed";
    return EXIT_FAILURE;
  }

  LOG_INFO << "User group map capacity: " << user_group_map_->max_size()
           << ", current size: " << user_group_map_->size();

  udp_server_.SetMessageCallback(std::bind(
      &MysticSalesmanSvrdApp::HandleUDPMessage, this, _1, _2, _3, _4));
  bool ok = udp_server_.Run(alpha::NetAddress("0.0.0.0", udp_port_));
  if (!ok) {
    PLOG_ERROR << "Start udp server failed";
    return EXIT_FAILURE;
  }

  http_server_.SetCallback(
      std::bind(&MysticSalesmanSvrdApp::HandleHTTPMessage, this, _1, _2));
  ok = http_server_.Run(alpha::NetAddress("0.0.0.0", http_port_));
  if (!ok) {
    PLOG_ERROR << "Start http server failed";
    return EXIT_FAILURE;
  }
  TrapSignals();
  loop_.Run();
  return EXIT_SUCCESS;
}

int MysticSalesmanSvrdApp::Daemonize() { return daemon(0, 0); }

void MysticSalesmanSvrdApp::TrapSignals() {
  auto ignore = [] {};
  auto quit = [this] { loop_.Quit(); };

  loop_.TrapSignal(SIGINT, quit);
  loop_.TrapSignal(SIGQUIT, quit);
  loop_.TrapSignal(SIGTERM, quit);

  loop_.TrapSignal(SIGHUP, ignore);
  loop_.TrapSignal(SIGCHLD, ignore);
  loop_.TrapSignal(SIGPIPE, ignore);
}

void MysticSalesmanSvrdApp::HandleUDPMessage(alpha::UDPSocket* socket,
                                             alpha::IOBuffer* buf,
                                             size_t buf_len,
                                             const alpha::NetAddress& peer) {
  MysticSalesmanServerProtocol::QueryUserGroup req;
  bool ok = req.ParseFromArray(buf->data(), buf_len);
  if (!ok) {
    LOG_WARNING << "Invalid request from client, HexDump:\n"
                << alpha::HexDump(alpha::Slice(buf->data(), buf_len));
    return;
  }
  MysticSalesmanServerProtocol::QueryUserGroupReply resp;
  resp.set_uin(req.uin());
  auto it = user_group_map_->find(req.uin());
  if (it == user_group_map_->end()) {
    resp.set_user_group(0);
  } else {
    resp.set_user_group(it->second);
  }
  DLOG_INFO << "Process uin " << req.uin() << ", group: " << resp.user_group();
  alpha::IOBufferWithSize out(resp.ByteSize());
  ok = resp.SerializeToArray(out.data(), out.size());
  CHECK(ok);
  int nwritten = socket->SendTo(&out, out.size(), peer);
  LOG_WARNING_IF(nwritten < 0) << "Send udp message to " << peer
                               << " failed, size: " << out.size();
}

void MysticSalesmanSvrdApp::HandleHTTPMessage(
    alpha::TcpConnectionPtr conn, const alpha::HTTPMessage& message) {
  alpha::HTTPResponseBuilder bad_request(conn);
  bad_request.status(400, "Bad Request");
  if (message.Method() != "POST") {
    LOG_INFO << "Unexpected method: " << message.Method();
    bad_request.SendWithEOM();
    return;
  }

  std::string file;
  const auto& payloads = message.payloads();
  if (payloads.empty()) {
    LOG_INFO << "No payload found, use message body instead";
    file = message.Body();
  } else {
    // Use first payload as upload file
    file = payloads[0].ToString();
  }

  boost::trim(file);
  std::istringstream iss(file);
  std::string line;
  std::map<uint32_t, uint32_t> new_user_group_map;
  while (std::getline(iss, line)) {
    boost::trim(line);
    uint32_t uin, group;
    int num = sscanf(line.c_str(), "%u %u", &uin, &group);
    if (num != 2) {
      LOG_INFO << "Invalid line in file: " << line;
      bad_request.SendWithEOM();
      return;
    }
    DLOG_INFO << "Read " << uin << " " << group;
    new_user_group_map[uin] = group;
  }

  user_group_map_->clear();
  for (const auto& p : new_user_group_map) {
    UserGroupMap::value_type pair;
    pair.first = p.first;
    pair.second = p.second;
    user_group_map_->insert(pair);
  }
  LOG_INFO << "User group map updated, new size: " << user_group_map_->size();
  alpha::HTTPResponseBuilder(conn).status(200, "OK").SendWithEOM();
}
