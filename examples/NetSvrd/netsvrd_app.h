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

#ifndef __NETSVRD_APP_H__
#define __NETSVRD_APP_H__

#include <memory>
#include <alpha/compiler.h>
#include <alpha/tcp_server.h>
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

#endif /* ----- #ifndef __NETSVRD_APP_H__  ----- */
