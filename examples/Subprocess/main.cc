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

#include <iostream>
#include <alpha/logger.h>
#include <alpha/Subprocess.h>

int main(int argc, char* argv[]) {
  alpha::Logger::Init(argv[0]);
  alpha::Logger::set_logtostderr(true);

  std::vector<std::string> args = {"/usr/bin/sleep", "3"};
  alpha::Subprocess sub(args);

  pid_t pid = sub.pid();
  DLOG_INFO << "Subproces id: " << pid;
  while (1) {
    auto rc = sub.Poll();
    if (rc.KilledBySignal()) {
      DLOG_INFO << "Child " << pid << " killed by signal "
                << rc.SignalReceived();
      break;
    } else if (rc.TerminatedNormally()) {
      DLOG_INFO << "Child " << pid << " exit with code " << rc.ExitCode();
      break;
    } else {
      DLOG_INFO << "Child is still running";
    }
    ::usleep(1000 * 300);
  }
}
