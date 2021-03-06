/*
 * =============================================================================
 *
 *       Filename:  netsvrd_app.h
 *        Created:  12/10/15 10:34:34
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include <memory>
#include <alpha/Compiler.h>
#include <alpha/TcpServer.h>
#include <alpha/File.h>

class NetSvrdVirtualServer;
class NetSvrdApp {
 public:
  NetSvrdApp(alpha::EventLoop* loop);
  ~NetSvrdApp();
  int Init(const char* file);
  int Run();

  DISABLE_COPY_ASSIGNMENT(NetSvrdApp);

 private:
  using NetSvrdVirtualServerPtr = std::unique_ptr<NetSvrdVirtualServer>;
  bool CreatePidFile();
  void TrapSignals();
  int Cron();

  alpha::EventLoop* loop_;
  uint64_t net_server_id_;
  alpha::File pid_file_;
  std::string pid_file_path_;
  std::vector<NetSvrdVirtualServerPtr> servers_;
};

