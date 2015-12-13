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
#include <alpha/logger.h>
#include <alpha/ProcessBus.h>
#include "netsvrd_frame.h"

int main(int argc, char* argv[]) {
  alpha::Logger::Init(argv[0]);
  if (argc != 4) {
    LOG_ERROR << "Usage: " << argv[0] << " [ServerID] [InputBus] [OutputBus]";
    return EXIT_FAILURE;
  }
  auto server_id = std::stoull(argv[1]);
  DLOG_INFO << server_id;
  (void)server_id;
  auto bus_in = alpha::ProcessBus::RestoreFrom(argv[2]);
  if (bus_in == nullptr) {
    LOG_ERROR << "Restore form " << argv[2] << " failed";
    return EXIT_FAILURE;
  }

  auto bus_out = alpha::ProcessBus::RestoreFrom(argv[3]);
  if (bus_out == nullptr) {
    LOG_ERROR << "Restore form " << argv[3] << " failed";
    return EXIT_FAILURE;
  }

  while (1) {
    int len;
    auto data = bus_in->Read(&len);
    if (data) {
      DLOG_INFO << "Receive data, len: " << len;
      auto internal_frame = reinterpret_cast<NetSvrdInternalFrame*>(data);
      DLOG_INFO << "Server id in frame: " << internal_frame->net_server_id;
      DLOG_INFO << "Client id in frame: " << internal_frame->client_id;
      bus_out->Write(data, len);
    }
    usleep(1000);
  }
}
