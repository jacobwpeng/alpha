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
#include <alpha/event_loop.h>
#include <alpha/logger.h>
#include <alpha/random.h>
#include "netsvrd_virtual_server.h"

using namespace std::placeholders;
NetSvrdApp::NetSvrdApp(alpha::EventLoop* loop)
    : loop_(loop), net_server_id_(alpha::Random::Rand64()) {}

NetSvrdApp::~NetSvrdApp() = default;

int NetSvrdApp::Init(const char* file) {
  using boost::property_tree::ptree;
  ptree pt;
  try {
    boost::property_tree::read_xml(
        file, pt, boost::property_tree::xml_parser::no_comments);
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
  bool ok = true;
  std::for_each(std::begin(servers_), std::end(servers_),
                [&ok](NetSvrdVirtualServerPtr& server) {
    if (ok) ok = server->Run();
  });
  loop_->set_cron_functor(std::bind(&NetSvrdApp::Cron, this));
  return ok ? loop_->Run(), 0 : -1;
}

int NetSvrdApp::Cron() {
  for (auto& server : servers_) {
    server->FlushWorkersOutput();
  }
  return 0;
}
