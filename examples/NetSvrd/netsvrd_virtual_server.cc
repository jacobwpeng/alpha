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
#include <alpha/Logger.h>
#include <alpha/EventLoop.h>
#include <alpha/TcpServer.h>
#include <alpha/IOBuffer.h>
#include <alpha/UDPServer.h>
#include "netsvrd_frame.h"

using namespace std::placeholders;
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
  if (parts[0] == "tcp") {
    server_type_ = NetSvrdVirtualServerType::kProtocolTCP;
  } else if (parts[0] == "udp") {
    server_type_ = NetSvrdVirtualServerType::kProtocolUDP;
  } else {
    LOG_ERROR << "Unknown protocol: " << parts[0] << ", support tcp/udp only";
    return false;
  }
  if (parts[1] == "*") {
    parts[1] = "0.0.0.0";
  }
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
      next_worker_index_(0),
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

  if (parser.server_type() == NetSvrdVirtualServerType::kProtocolTCP) {
    tcp_servers_.emplace_back(
        alpha::make_unique<alpha::TcpServer>(loop_, parser.address()));
    auto& server = *tcp_servers_.rbegin();
    server->SetOnNewConnection(
        std::bind(&NetSvrdVirtualServer::OnConnected, this, _1));
    server->SetOnRead(
        std::bind(&NetSvrdVirtualServer::OnMessage, this, _1, _2));
    server->SetOnClose(std::bind(&NetSvrdVirtualServer::OnClose, this, _1));
  } else if (parser.server_type() == NetSvrdVirtualServerType::kProtocolUDP) {
    auto p = udp_servers_.emplace(parser.address(),
                                  alpha::make_unique<alpha::UDPServer>(loop_));
    CHECK(p.second);
    auto& server = p.first->second;
    server->SetMessageCallback(
        std::bind(&NetSvrdVirtualServer::OnUDPMessage, this, _1, _2, _3, _4));
  }
  return true;
}

bool NetSvrdVirtualServer::Run() {
  for (auto& server : tcp_servers_) {
    if (!server->Run()) return false;
  }

  for (auto& p : udp_servers_) {
    if (!p.second->Run(p.first)) return false;
  }

  for (auto i = 0u; i < max_worker_num_; ++i) {
    workers_.emplace_back(SpawnWorker(i));
  }
  StartMonitorWorkers();
  return true;
}

void NetSvrdVirtualServer::FlushWorkersOutput() {
  char* data = nullptr;
  int len = 0;
  for (auto& worker : workers_) {
    do {
      data = worker->BusOut()->Read(&len);
      if (data) {
        DLOG_INFO << "Data len from worker: " << len;
        auto internal_frame = reinterpret_cast<NetSvrdInternalFrame*>(data);
        if (internal_frame->net_server_id != net_server_id_) {
          LOG_WARNING << "Drop obsolete frame, old server id: "
                      << internal_frame->net_server_id;
          continue;
        }
        auto client_id = internal_frame->client_id;
        if (client_id >= next_connection_id_) {
          LOG_WARNING << "Invalid client id found, id: " << client_id;
          continue;
        }
        auto it = connections_.find(client_id);
        if (it == connections_.end()) {
          LOG_INFO << "Frame to closed connection, id: " << client_id;
          continue;
        }
        bool ok = it->second->Write(alpha::Slice(data, len));
        if (!ok) {
          LOG_WARNING << "Write to client failed";
          continue;
        }
      }
    } while (data);
  }
}

void NetSvrdVirtualServer::OnConnected(alpha::TcpConnectionPtr conn) {
  NetSvrdConnectionContext ctx;
  ctx.connection_id_ = next_connection_id_++;
  ctx.codec = std::make_shared<NetSvrdFrameCodec>();
  conn->SetContext(ctx);
  auto p = connections_.emplace(ctx.connection_id_, conn.get());
  CHECK(p.second);
  LOG_INFO << "New connection from " << conn->PeerAddr()
           << ", id: " << ctx.connection_id_;
}

void NetSvrdVirtualServer::OnMessage(alpha::TcpConnectionPtr conn,
                                     alpha::TcpConnectionBuffer* buffer) {
  auto ctx = conn->GetContextPtr<NetSvrdConnectionContext>();
  CHECK(ctx);
  while (buffer->Read().size() >= NetSvrdFrame::kHeaderSize) {
    auto frame = ctx->codec->OnMessage(conn, buffer);
    if (conn->closed()) break;
    if (frame) {
      OnFrame(ctx->connection_id_, std::move(frame));
    }
  }
}

void NetSvrdVirtualServer::OnClose(alpha::TcpConnectionPtr conn) {
  auto ctx = conn->GetContextPtr<NetSvrdConnectionContext>();
  CHECK(ctx);
  auto num = connections_.erase(ctx->connection_id_);
  CHECK(num == 1);
  LOG_INFO << "Connection closed, id: " << ctx->connection_id_
           << ", addr: " << conn->PeerAddr();
}

void NetSvrdVirtualServer::OnUDPMessage(alpha::UDPSocket* socket,
                                        alpha::IOBuffer* buf,
                                        size_t buf_len,
                                        const alpha::NetAddress& address) {
  auto header = NetSvrdFrame::CastHeaderOnly(buf->data(), buf_len);
  if (header == nullptr) {
    LOG_ERROR << "Receive invalid UDP message from " << address;
    return;
  }
  if (header->size() != buf_len) {
    LOG_ERROR << "UDP message size mismatch";
    return;
  }
  // TODO: OnFrame with local frame
  auto frame = NetSvrdFrame::CreateUnique(header->payload_size);
  memcpy(frame->payload,
         buf->data() + NetSvrdFrame::kHeaderSize,
         header->payload_size);
  OnFrame(0, std::move(frame));
}

void NetSvrdVirtualServer::OnFrame(uint64_t connection_id,
                                   NetSvrdFrame::UniquePtr&& frame) {
  DLOG_INFO << "Frame payload size: " << frame->payload_size;
  auto p = frame.get();
  auto internal_frame = reinterpret_cast<NetSvrdInternalFrame*>(p);
  internal_frame->client_id = connection_id;
  internal_frame->net_server_id = net_server_id_;
  auto worker = NextWorker();
  CHECK(worker);
  bool ok = worker->BusIn()->Write(reinterpret_cast<const char*>(frame->data()),
                                   frame->size());
  LOG_WARNING_IF(!ok) << "Write to worker input bus failed, drop it";
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

NetSvrdWorkerPtr NetSvrdVirtualServer::SpawnWorker(int worker_id) {
  std::ostringstream oss;
  oss << bus_dir_ << '/' << "worker_" << worker_id << "_in.bus";
  std::string bus_in_path = oss.str();
  oss.str("");
  oss << bus_dir_ << '/' << "worker_" << worker_id << "_out.bus";
  std::string bus_out_path = oss.str();

  static const size_t kProcessBusSize = 1 << 20;
  auto bus_in =
      alpha::ProcessBus::RestoreOrCreate(bus_in_path, kProcessBusSize, true);
  CHECK(bus_in);
  auto bus_out =
      alpha::ProcessBus::RestoreOrCreate(bus_out_path, kProcessBusSize, true);
  CHECK(bus_out);
  DLOG_INFO << "Create worker , path: " << worker_path_;
  std::vector<std::string> argv = {
      worker_path_, std::to_string(net_server_id_), bus_in_path, bus_out_path};
  alpha::Subprocess::Options options;
  options.CloseOtherFds();
  return alpha::make_unique<NetSvrdWorker>(
      alpha::make_unique<alpha::Subprocess>(argv, nullptr, options),
      std::move(bus_in),
      std::move(bus_out));
}

void NetSvrdVirtualServer::PollWorkers() {
  CHECK(workers_.size() == max_worker_num_);
  for (auto i = 0u; i < max_worker_num_; ++i) {
    auto& worker = workers_[i];
    auto rc = worker->Process()->Poll();
    if (rc.Running()) continue;

    LOG_WARNING << "Worker is " << rc.status() << ", path: " << worker_path_;
    worker = std::move(SpawnWorker(i));
  }
}

NetSvrdWorker* NetSvrdVirtualServer::NextWorker() {
  CHECK(next_worker_index_ < workers_.size());
  auto worker = workers_[next_worker_index_].get();
  ++next_worker_index_;
  if (next_worker_index_ == workers_.size()) {
    next_worker_index_ = 0;
  }
  return worker;
}
