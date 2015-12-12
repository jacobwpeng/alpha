/*
 * =============================================================================
 *
 *       Filename:  Subprocess.cc
 *        Created:  12/10/15 10:54:44
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include "Subprocess.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <algorithm>
#include "logger.h"

namespace alpha {
SubprocessReturnCode::State SubprocessReturnCode::state() const {
  if (raw_status_ == kRawStatusNotStarted) return State::kNotStarted;
  if (raw_status_ == kRawStatusRunning) return State::kRunning;
  if (WIFEXITED(raw_status_)) return State::kExited;
  if (WIFSIGNALED(raw_status_)) return State::kKilledBySignal;
  CHECK(false) << "Invalid raw_status_: " << raw_status_;
  return State::kNotStarted;
}

std::string SubprocessReturnCode::status() const {
  switch (state()) {
    case State::kRunning:
      return "running";
    case State::kNotStarted:
      return "not started";
    case State::kExited:
      return "exited with code " + std::to_string(ExitCode());
    case State::kKilledBySignal:
      return "killed by signal " + std::to_string(SignalReceived());
    default:
      CHECK(false) << "Invalid state: " << state();
      return "";
  }
}

bool SubprocessReturnCode::NotStarted() const {
  return state() == State::kNotStarted;
}

bool SubprocessReturnCode::Running() const {
  return state() == State::kRunning;
}

bool SubprocessReturnCode::TerminatedNormally() const {
  return state() == State::kExited;
}

int SubprocessReturnCode::ExitCode() const {
  CHECK(TerminatedNormally());
  return WEXITSTATUS(raw_status_);
}

bool SubprocessReturnCode::KilledBySignal() const {
  return state() == State::kKilledBySignal;
}

int SubprocessReturnCode::SignalReceived() const {
  CHECK(KilledBySignal());
  return WTERMSIG(raw_status_);
}

Subprocess::Subprocess(const std::vector<std::string>& argv,
                       const char* executable)
    : pid_(0) {
  CHECK(!argv.empty()) << "no argv";
  if (executable == nullptr) {
    executable = argv[0].data();
  }
  DLOG_INFO << "Before fork";
  auto rc = fork();
  if (rc < 0) {
    throw SubprocessSpawnError("fork failed", errno);
  } else if (rc == 0) {
    std::vector<char*> args;
    std::transform(
        std::begin(argv), std::end(argv), std::back_inserter(args),
        [](const std::string& arg) { return const_cast<char*>(arg.data()); });
    args.push_back(nullptr);
    rc = execv(executable, args.data());
    PLOG_ERROR << "execv";
    throw SubprocessSpawnError("execv failed", errno);
  } else {
    return_code_.SetRunning();
    pid_ = rc;
  }
}

SubprocessReturnCode Subprocess::Wait() {
  CHECK(return_code_.Running());
  int raw_status;
  int found = 0;
  do {
    found = waitpid(pid_, &raw_status, 0);
  } while (found == -1 && errno == EINTR);
  PCHECK(found == pid_) << "waitpid failed";
  return_code_ = SubprocessReturnCode(raw_status);
  return return_code_;
}

SubprocessReturnCode Subprocess::Poll() {
  CHECK(return_code_.Running());
  int raw_status;
  int found = 0;
  do {
    found = waitpid(pid_, &raw_status, WNOHANG);
  } while (found == -1 && errno == EINTR);
  PCHECK(found != -1) << "waitpid(" << pid_ << ", &raw_status, WNOHANG)";
  if (found != 0) {
    return_code_ = SubprocessReturnCode(raw_status);
  }
  return return_code_;
}
}
