/*
 * =============================================================================
 *
 *       Filename:  SectMemberCacheServerMain.cc
 *        Created:  04/25/16 14:42:33
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "SectMemberCacheServerApp.h"
#include <string>
#include <iostream>
#include <alpha/Logger.h>

static int Usage(const char* argv0) {
  std::cout << "Usage: " << argv0 << " ip port\n";
  return EXIT_FAILURE;
}

int main(int argc, char* argv[]) {
  if (argc != 3) {
    return Usage(argv[0]);
  }
  alpha::Logger::Init(argv[0]);
  int port = std::stoi(argv[2]);
  SectMemberCacheServerApp app(argv[1], port);
  return app.Run();
}
