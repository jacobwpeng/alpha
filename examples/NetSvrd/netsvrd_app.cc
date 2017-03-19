/*
 * =============================================================================
 *
 *       Filename:  netsvrd_app.cc
 *        Created:  12/10/15 10:43:31
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "netsvrd_app.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <alpha/EventLoop.h>
#include <alpha/Logger.h>
#include <alpha/Random.h>
#include <alpha/FileUtil.h>
#include "netsvrd_virtual_server.h"

using namespace std::placeholders;
NetSvrdApp::NetSvrdApp(alpha::EventLoop* loop)
    : loop_(loop), net_server_id_(alpha::Random::Rand64()) {}

NetSvrdApp::~NetSvrdApp() {
  if (pid_file_) {
    alpha::DeleteFile(pid_file_path_);
  }
}

int NetSvrdApp::Init(const char* file) {
  using boost::property_tree::ptree;
  ptree pt;
  try {
    boost::property_tree::read_xml(
        file, pt, boost::property_tree::xml_parser::no_comments);
    pid_file_path_ = pt.get<std::string>("NetSvrd.PidFile.<xmlattr>.path");
    for (const auto& child : pt.get_child("Servers")) {
      if (child.first != "Server") continue;
      auto server = child.second;
      auto bus_dir =
          server.get_child("BusDir").get<std::string>("<xmlattr>.path");
      auto worker = server.get_child("Worker");
      auto worker_path = worker.get<std::string>("<xmlattr>.path");
      auto worker_max_num = worker.get<unsigned>("<xmlattr>.max_num");
      auto virtual_server = alpha::make_unique<NetSvrdVirtualServer>(
          net_server_id_, loop_, bus_dir, worker_path, worker_max_num);
      for (const auto& interface : server.get_child("")) {
        if (interface.first != "Interface") continue;
        virtual_server->AddInterface(
            interface.second.get<std::string>("<xmlattr>.addr"));
      }
      servers_.emplace_back(std::move(virtual_server));
    }
  } catch (std::exception& e) {
    LOG_ERROR << "NetSvrdApp::Init failed, file: " << file << ", " << e.what();
    return -1;
  }
  return 0;
}

int NetSvrdApp::Run() {
  int err = daemon(0, 0);
  if (err) {
    PLOG_ERROR << "daemon";
    return err;
  }
  if (!CreatePidFile()) {
    return EXIT_FAILURE;
  }
  TrapSignals();
  bool ok = true;
  std::for_each(std::begin(servers_),
                std::end(servers_),
                [&ok](NetSvrdVirtualServerPtr& server) {
                  if (ok) ok = server->Run();
                });
  loop_->set_cron_functor(std::bind(&NetSvrdApp::Cron, this));
  return ok ? loop_->Run(), 0 : -1;
}

bool NetSvrdApp::CreatePidFile() {
  alpha::File file(pid_file_path_, O_WRONLY | O_CREAT);
  if (!file) {
    PLOG_ERROR << "Open pid file failed";
    return false;
  }
  if (!file.TryLock()) {
    PLOG_ERROR << "Lock pid file failed";
    return false;
  }
  auto pid = std::to_string(getpid());
  int rc = file.Write(pid.c_str(), pid.size());
  if (rc != static_cast<int>(pid.size())) {
    PLOG_ERROR << "Write pid to file failed";
    return false;
  }
  pid_file_ = std::move(file);
  return true;
}

void NetSvrdApp::TrapSignals() {
  auto ignore = [] {};
  auto quit = [this] { loop_->Quit(); };

  loop_->TrapSignal(SIGINT, quit);
  loop_->TrapSignal(SIGQUIT, quit);
  loop_->TrapSignal(SIGTERM, quit);

  loop_->TrapSignal(SIGHUP, ignore);
  loop_->TrapSignal(SIGCHLD, ignore);
  loop_->TrapSignal(SIGPIPE, ignore);
}

int NetSvrdApp::Cron() {
  for (auto& server : servers_) {
    server->FlushWorkersOutput();
  }
  return 0;
}
