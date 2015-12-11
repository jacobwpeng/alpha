/*
 * =============================================================================
 *
 *       Filename:  main.cc
 *        Created:  12/10/15 11:27:09
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <alpha/logger.h>
#include <alpha/Subprocess.h>
#include <iostream>

int main(int argc, char* argv[]) {
  alpha::Logger::Init(argv[0]);
  alpha::Logger::set_logtostderr(true);

  std::vector<std::string> args = {"/usr/bin/sleep", "100"};
  alpha::Subprocess sub(args);

  pid_t pid = sub.pid();
  auto rc = sub.Poll();
  if (rc.KilledBySignal()) {
    DLOG_INFO << "Child " << pid << " killed by signal " << rc.SignalReceived();
  } else if (rc.TerminatedNormally()) {
    DLOG_INFO << "Child " << pid << " exit with code " << rc.ExitCode();
  } else {
    DLOG_INFO << "Child is still running";
  }
}
