/*
 * =============================================================================
 *
 *       Filename:  netsvrd_main.cc
 *        Created:  12/10/15 10:34:16
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <iostream>
#include <alpha/event_loop.h>
#include <alpha/logger.h>
#include "netsvrd_app.h"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cout << "Usage: " << argv[0] << " Conf\n";
    return EXIT_FAILURE;
  }
  alpha::Logger::Init(argv[0]);

  alpha::EventLoop loop;
  NetSvrdApp app(&loop);
  int err = app.Init(argv[1]);
  return err ? EXIT_FAILURE : app.Run();
}
