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

#include <map>
#include <vector>
#include <string>
#include <alpha/compiler.h>
#include <alpha/tcp_connection.h>
#include <alpha/timer_manager.h>
#include <alpha/Subprocess.h>
#include "netsvrd_frame_codec.h"
#include "netsvrd_worker.h"

namespace alpha {
class IOBuffer;
class EventLoop;
class TcpServer;
class UDPServer;
class UDPSocket;
}

enum class NetSvrdVirtualServerType : uint8_t {
  kProtocolUnknown = 0,
  kProtocolTCP = 1,
  kProtocolUDP = 2,
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
  void FlushWorkersOutput();

 private:
  using TcpServerPtr = std::unique_ptr<alpha::TcpServer>;
  using UDPServerPtr = std::unique_ptr<alpha::UDPServer>;
  static const uint32_t kCheckWorkerStatusInterval = 1000;  // 1000 ms
  void OnConnected(alpha::TcpConnectionPtr conn);
  void OnMessage(alpha::TcpConnectionPtr conn,
                 alpha::TcpConnectionBuffer* buffer);
  void OnClose(alpha::TcpConnectionPtr conn);
  void OnUDPMessage(alpha::UDPSocket* socket, alpha::IOBuffer* buf,
                    size_t buf_len, const alpha::NetAddress& address);
  void OnFrame(uint64_t connection_id, NetSvrdFrame::UniquePtr&& frame);
  void StartMonitorWorkers();
  void StopMonitorWorkers();
  NetSvrdWorkerPtr SpawnWorker(int worker_id);
  void PollWorkers();
  NetSvrdWorker* NextWorker();
  alpha::EventLoop* loop_;
  unsigned max_worker_num_;
  size_t next_worker_index_;
  uint64_t net_server_id_;
  uint64_t next_connection_id_;
  alpha::TimerManager::TimerId poll_workers_timer_id_;
  std::string bus_dir_;
  std::string worker_path_;
  std::vector<TcpServerPtr> tcp_servers_;
  std::map<alpha::NetAddress, UDPServerPtr> udp_servers_;
  std::vector<NetSvrdWorkerPtr> workers_;
  std::map<uint64_t, alpha::TcpConnection*> connections_;
};

#endif /* ----- #ifndef __NETSVRD_VIRTUAL_SERVER_H__  ----- */
