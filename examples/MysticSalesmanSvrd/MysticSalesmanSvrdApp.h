/*
 * =============================================================================
 *
 *       Filename:  MysticSalesmanSvrdApp.h
 *        Created:  12/18/15 15:00:28
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include <cstdint>
#include <memory>
#include <alpha/Slice.h>
#include <alpha/EventLoop.h>
#include <alpha/UDPServer.h>
#include <alpha/SimpleHTTPServer.h>
#include <alpha/MemoryMappedFile.h>
#include <alpha/experimental/RegionBasedHashMap.h>

class MysticSalesmanSvrdApp final {
 public:
  MysticSalesmanSvrdApp(int udp_port,
                        int http_port,
                        const char* mmap_file_path);
  int Run();

 private:
  static const size_t kMMapFileSize = 10 << 20;
  struct UserSalesInfo {
    unsigned group;
    unsigned from;
    unsigned to;
  };
  using UserGroupMap = alpha::RegionBasedHashMap<uint32_t, UserSalesInfo>;
  int Daemonize();
  void TrapSignals();
  void HandleUDPMessage(alpha::UDPSocket* socket,
                        alpha::IOBuffer* buf,
                        size_t buf_len,
                        const alpha::NetAddress& peer);
  void HandleHTTPMessage(alpha::TcpConnectionPtr,
                         const alpha::HTTPMessage& message);
  int udp_port_;
  int http_port_;
  alpha::EventLoop loop_;
  std::string mmap_file_path_;
  alpha::MemoryMappedFile mmap_file_;
  std::unique_ptr<UserGroupMap> user_group_map_;
  alpha::UDPServer udp_server_;
  alpha::SimpleHTTPServer http_server_;
};
