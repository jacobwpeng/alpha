/*
 * =============================================================================
 *
 *       Filename:  SectInfoServerMain.cc
 *        Created:  04/15/16 15:36:30
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <iostream>
#include <alpha/logger.h>
#include "SectInfoServerApp.h"

int Usage(const char* argv0) {
  std::cout << "Usage: " << argv0 << " [Conf]\n";
  return EXIT_FAILURE;
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    return Usage(argv[0]);
  }
  alpha::Logger::Init(argv[0]);
  SectInfoServerApp app;
  int err = app.Init(argv[1]);
  if (err) {
    LOG_ERROR << "Init app failed, err: " << err;
    return EXIT_FAILURE;
  }
  return app.Run();
}
