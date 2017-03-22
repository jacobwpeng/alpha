/*
 * =============================================================================
 *
 *       Filename:  netsvrd_echo_worker.cc
 *        Created:  12/11/15 20:39:09
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <unistd.h>
#include <iostream>
#include <alpha/Logger.h>
#include <alpha/ProcessBus.h>
#include "netsvrd_frame.h"

int main(int argc, char* argv[]) {
  alpha::Logger::Init(argv[0]);
  if (argc != 3) {
    LOG_ERROR << "Usage: " << argv[0] << " [ServerID] [BusPath]";
    return EXIT_FAILURE;
  }
  auto server_id = std::stoull(argv[1]);
  DLOG_INFO << "Server ID: " << server_id;
  alpha::ProcessBus bus;
  if (!bus.RestoreFrom(argv[2], alpha::ProcessBus::QueueOrder::kWriteFirst)) {
    LOG_ERROR << "Restore bus from " << argv[2] << " failed";
    return EXIT_FAILURE;
  }

  while (1) {
    int len;
    auto data = bus.Read(&len);
    if (data) {
      DLOG_INFO << "Receive data, len: " << len;
      auto internal_frame = reinterpret_cast<NetSvrdInternalFrame*>(data);
      DLOG_INFO << "Server id in frame: " << internal_frame->net_server_id;
      DLOG_INFO << "Client id in frame: " << internal_frame->client_id;
      bus.Write(data, len);
    }
    usleep(1000);
  }
}
