/*
 * =============================================================================
 *
 *       Filename:  netsvrd_virtual_server.cc
 *        Created:  12/10/15 15:02:47
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "netsvrd_virtual_server.h"
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <alpha/logger.h>
#include <alpha/event_loop.h>
#include <alpha/tcp_server.h>
#include "netsvrd_frame.h"

NetSvrdAddressParser::NetSvrdAddressParser(const std::string& addr)
    : valid_(false), server_type_(NetSvrdVirtualServerType::kProtocolUnknown) {
  valid_ = Parse(addr);
}

bool NetSvrdAddressParser::Parse(const std::string& addr) {
  std::vector<std::string> parts;
  boost::split(parts, addr, boost::is_any_of("/:"), boost::token_compress_on);
  if (parts.size() != 3) {
    LOG_ERROR << "Invalid addr: " << addr;
    return false;
  }
  if (parts[0] != "tcp") {
    LOG_ERROR << "Unknown protocol: " << parts[0] << ", Only tcp is supported";
    return false;
  }
  if (parts[1] == "*") {
    parts[1] = "0.0.0.0";
  }

  server_type_ = NetSvrdVirtualServerType::kProtocolTcp;
  try {
    server_address_ = alpha::NetAddress(parts[1], stoul(parts[2]));
  } catch (std::exception& e) {
    LOG_ERROR << "Convert " << parts[2] << " to integer failed, " << e.what();
    return false;
  }
  return true;
}

NetSvrdVirtualServer::NetSvrdVirtualServer(uint64_t net_server_id,
                                           alpha::EventLoop* loop,
                                           const std::string& bus_dir,
                                           const std::string& worker_path,
                                           unsigned max_worker_num)
    : loop_(loop),
      max_worker_num_(max_worker_num),
      net_server_id_(net_server_id),
      next_connection_id_(1),
      poll_workers_timer_id_(0),
      bus_dir_(bus_dir),
      worker_path_(worker_path) {}

NetSvrdVirtualServer::~NetSvrdVirtualServer() {
  // loop_->RemoveTimer(poll_workers_timer_id_);
}

bool NetSvrdVirtualServer::AddInterface(const std::string& addr) {
  NetSvrdAddressParser parser(addr);
  if (!parser.valid()) {
    LOG_ERROR << "Invalid addr: " << addr;
    return false;
  }

  if (parser.server_type() == NetSvrdVirtualServerType::kProtocolTcp) {
    tcp_servers_.emplace_back(
        alpha::make_unique<alpha::TcpServer>(loop_, parser.address()));
  }
  return true;
}

bool NetSvrdVirtualServer::Run() {
  for (auto it = std::begin(tcp_servers_); it != std::end(tcp_servers_); ++it) {
    if (!(*it)->Run()) {
      return false;
    }
  }
  for (auto i = 0u; i < max_worker_num_; ++i) {
    workers_.emplace_back(SpawnWorker(i));
  }
  StartMonitorWorkers();
  return true;
}

void NetSvrdVirtualServer::OnConnected(alpha::TcpConnectionPtr conn) {
  using namespace std::placeholders;
  NetSvrdConnectionContext ctx;
  ctx.connection_id_ = next_connection_id_++;
  ctx.codec = std::make_shared<NetSvrdFrameCodec>();
  conn->SetContext(ctx);
}

void NetSvrdVirtualServer::OnMessage(alpha::TcpConnectionPtr conn,
                                     alpha::TcpConnectionBuffer* buffer) {
  auto ctx = conn->GetContext<NetSvrdConnectionContext>();
  CHECK(ctx);
  auto frame = ctx->codec->OnMessage(conn, buffer);
  if (frame) {
    OnFrame(ctx->connection_id_, std::move(frame));
  }
}

void NetSvrdVirtualServer::OnFrame(uint64_t connection_id,
                                   std::unique_ptr<NetSvrdFrame>&& frame) {
  auto p = frame.get();
  auto internal_frame = reinterpret_cast<NetSvrdInternalFrame*>(p);
  internal_frame->client_id = connection_id;
  internal_frame->net_server_id = net_server_id_;
  DLOG_INFO << "Frame payload size: " << frame->payload_size;
}

void NetSvrdVirtualServer::StartMonitorWorkers() {
  using namespace std::placeholders;
  CHECK(poll_workers_timer_id_ == 0);
  poll_workers_timer_id_ =
      loop_->RunEvery(kCheckWorkerStatusInterval,
                      std::bind(&NetSvrdVirtualServer::PollWorkers, this));
}

void NetSvrdVirtualServer::StopMonitorWorkers() {
  CHECK(poll_workers_timer_id_ != 0);
  loop_->RemoveTimer(poll_workers_timer_id_);
  poll_workers_timer_id_ = 0;
}

NetSvrdVirtualServer::SubprocessPtr NetSvrdVirtualServer::SpawnWorker(
    int worker_id) {
  std::ostringstream oss;
  oss << bus_dir_ << '/' << "worker_" << worker_id << "_in.bus";
  std::string bus_in = oss.str();
  oss.str("");
  oss << bus_dir_ << '/' << "worker_" << worker_id << "_out.bus";
  std::string bus_out = oss.str();
  std::vector<std::string> argv = {worker_path_, std::to_string(net_server_id_),
                                   bus_in, bus_out};
  return alpha::make_unique<alpha::Subprocess>(argv);
}

void NetSvrdVirtualServer::PollWorkers() {
  CHECK(workers_.size() == max_worker_num_);
  for (auto i = 0u; i < max_worker_num_; ++i) {
    auto& worker = workers_[i];
    auto rc = worker->Poll();
    if (rc.Running()) continue;

    LOG_WARNING << "Worker is " << rc.status();
    worker = std::move(SpawnWorker(i));
  }
}
