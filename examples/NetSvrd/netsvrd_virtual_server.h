/*
 * =============================================================================
 *
 *       Filename:  netsvrd_virtual_server.h
 *        Created:  12/10/15 14:58:47
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#ifndef __NETSVRD_VIRTUAL_SERVER_H__
#define __NETSVRD_VIRTUAL_SERVER_H__

#include <vector>
#include <string>
#include <alpha/compiler.h>
#include <alpha/tcp_connection.h>
#include <alpha/timer_manager.h>
#include <alpha/Subprocess.h>
#include "netsvrd_frame_codec.h"

namespace alpha {
class EventLoop;
class TcpServer;
}

enum class NetSvrdVirtualServerType : uint8_t {
  kProtocolUnknown = 0,
  kProtocolTcp = 1
};

struct NetSvrdFrame;
class NetSvrdAddressParser {
 public:
  NetSvrdAddressParser(const std::string& addr);
  bool valid() const { return valid_; }

  NetSvrdVirtualServerType server_type() const { return server_type_; }
  const alpha::NetAddress& address() const { return server_address_; }

 private:
  bool Parse(const std::string& addr);
  bool valid_;
  NetSvrdVirtualServerType server_type_;
  alpha::NetAddress server_address_;
};

struct NetSvrdConnectionContext {
  uint64_t connection_id_;
  std::shared_ptr<NetSvrdFrameCodec> codec;
};

class NetSvrdVirtualServer final {
 public:
  NetSvrdVirtualServer(uint64_t net_server_id, alpha::EventLoop* loop,
                       const std::string& bus_dir,
                       const std::string& worker_path, unsigned max_worker_num);
  ~NetSvrdVirtualServer();
  DISABLE_COPY_ASSIGNMENT(NetSvrdVirtualServer);

  bool AddInterface(const std::string& addr);
  bool Run();

 private:
  using SubprocessPtr = std::unique_ptr<alpha::Subprocess>;
  using TcpServerPtr = std::unique_ptr<alpha::TcpServer>;
  static const uint32_t kCheckWorkerStatusInterval = 1000;  // 1000 ms
  void OnConnected(alpha::TcpConnectionPtr conn);
  void OnMessage(alpha::TcpConnectionPtr conn,
                 alpha::TcpConnectionBuffer* buffer);
  void OnFrame(uint64_t connection_id, std::unique_ptr<NetSvrdFrame>&& frame);
  void StartMonitorWorkers();
  void StopMonitorWorkers();
  SubprocessPtr SpawnWorker(int worker_id);
  void PollWorkers();
  alpha::EventLoop* loop_;
  unsigned max_worker_num_;
  uint64_t net_server_id_;
  uint64_t next_connection_id_;
  alpha::TimerManager::TimerId poll_workers_timer_id_;
  std::string bus_dir_;
  std::string worker_path_;
  std::vector<TcpServerPtr> tcp_servers_;
  std::vector<SubprocessPtr> workers_;
};

#endif /* ----- #ifndef __NETSVRD_VIRTUAL_SERVER_H__  ----- */
