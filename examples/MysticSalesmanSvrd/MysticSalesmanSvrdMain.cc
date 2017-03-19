/*
 * =============================================================================
 *
 *       Filename:  MysticSalesmanSvrdMain.cc
 *        Created:  12/18/15 15:00:13
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <cstdio>
#include <alpha/Logger.h>
#include "MysticSalesmanSvrdApp.h"

static int Usage(const char* prog) {
  fprintf(stderr, "Usage: %s UDP-Port HTTP-Port MMapFile-Path\n", prog);
  return EXIT_FAILURE;
}

static bool ReadPort(const char* s, int* port) {
  int num = sscanf(s, "%d", port);
  return num == 1;
}

int main(int argc, char* argv[]) {
  if (argc != 4) {
    return Usage(argv[0]);
  }
  // alpha::Logger::set_logtostderr(true);
  alpha::Logger::Init(argv[0]);
  int udp_port, http_port;

  if (ReadPort(argv[1], &udp_port) == false) {
    return Usage(argv[0]);
  }
  if (ReadPort(argv[2], &http_port) == false) {
    return Usage(argv[0]);
  }

  MysticSalesmanSvrdApp app(udp_port, http_port, argv[3]);
  return app.Run();
}
