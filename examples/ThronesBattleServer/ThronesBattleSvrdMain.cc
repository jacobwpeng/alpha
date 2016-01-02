/*
 * =============================================================================
 *
 *       Filename:  ThronesBattleSvrdMain.cc
 *        Created:  12/14/15 16:16:08
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <cstdio>
#include <alpha/logger.h>
#include "ThronesBattleSvrdApp.h"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s ConfFile\n", argv[0]);
    return EXIT_FAILURE;
  }
  alpha::Logger::set_logtostderr(true);
  alpha::Logger::Init(argv[0]);
  ThronesBattle::ServerApp app;
  int err = app.Init(argv[1]);
  if (err) {
    return err;
  }
  return app.Run();
}
