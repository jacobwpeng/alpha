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

#include <alpha/Subprocess.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <algorithm>
#include <alpha/Logger.h>

namespace alpha {
ProcessReturnCode::State ProcessReturnCode::state() const {
  if (raw_status_ == kRawStatusNotStarted) return State::kNotStarted;
  if (raw_status_ == kRawStatusRunning) return State::kRunning;
  if (WIFEXITED(raw_status_)) return State::kExited;
  if (WIFSIGNALED(raw_status_)) return State::kKilledBySignal;
  CHECK(false) << "Invalid raw_status_: " << raw_status_;
  return State::kNotStarted;
}

std::string ProcessReturnCode::status() const {
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

bool ProcessReturnCode::NotStarted() const {
  return state() == State::kNotStarted;
}

bool ProcessReturnCode::Running() const { return state() == State::kRunning; }

bool ProcessReturnCode::TerminatedNormally() const {
  return state() == State::kExited;
}

int ProcessReturnCode::ExitCode() const {
  CHECK(TerminatedNormally());
  return WEXITSTATUS(raw_status_);
}

bool ProcessReturnCode::KilledBySignal() const {
  return state() == State::kKilledBySignal;
}

int ProcessReturnCode::SignalReceived() const {
  CHECK(KilledBySignal());
  return WTERMSIG(raw_status_);
}

Subprocess::Options::Options() = default;

Subprocess::Options& Subprocess::Options::fd(int fd, int action) {
  fd_actions_[fd] = action;
  return *this;
}

Subprocess::Options& Subprocess::Options::CloseOtherFds() {
  close_other_fds_ = true;
  return *this;
}

Subprocess::Options& Subprocess::Options::operator|=(
    const Subprocess::Options& other) {
  if (this == &other) return *this;
  close_other_fds_ |= other.close_other_fds_;
  for (const auto& p : other.fd_actions_) {
    fd_actions_[p.first] = p.second;
  }
  return *this;
}

Subprocess::Subprocess(const std::vector<std::string>& argv,
                       const char* executable,
                       const Subprocess::Options& options)
    : pid_(0) {
  CHECK(!argv.empty()) << "no argv";
  if (executable == nullptr) {
    executable = argv[0].data();
  }
  auto rc = fork();
  if (rc < 0) {
    throw SubprocessSpawnError("fork failed", errno);
  } else if (rc == 0) {
    int err = PrepareChild(argv, executable, options);
    if (err) {
      throw SubprocessSpawnError("PrepareChild", err);
    }
  } else {
    return_code_.SetRunning();
    pid_ = rc;
  }
}

ProcessReturnCode Subprocess::Wait() {
  CHECK(return_code_.Running());
  int raw_status;
  int found = 0;
  do {
    found = waitpid(pid_, &raw_status, 0);
  } while (found == -1 && errno == EINTR);
  PCHECK(found == pid_) << "waitpid failed";
  return_code_ = ProcessReturnCode(raw_status);
  return return_code_;
}

ProcessReturnCode Subprocess::Poll() {
  CHECK(return_code_.Running());
  int raw_status;
  int found = 0;
  do {
    found = waitpid(pid_, &raw_status, WNOHANG);
  } while (found == -1 && errno == EINTR);
  PCHECK(found != -1) << "waitpid(" << pid_ << ", &raw_status, WNOHANG)";
  if (found != 0) {
    return_code_ = ProcessReturnCode(raw_status);
  }
  return return_code_;
}

int Subprocess::PrepareChild(const std::vector<std::string>& argv,
                             const char* executable,
                             const Subprocess::Options& options) {
  for (int i = 0; i < NSIG; ++i) {
    signal(i, SIG_DFL);
  }
  for (const auto& p : options.fd_actions_) {
    if (p.second == Options::kActionClose) {
      ::close(p.second);
    }
  }
  if (options.close_other_fds_) {
    for (int fd = getdtablesize() - 1; fd >= 3; --fd) {
      if (options.fd_actions_.count(fd) == 0) {
        ::close(fd);
      }
    }
  }
  std::vector<char*> args;
  std::transform(
      std::begin(argv),
      std::end(argv),
      std::back_inserter(args),
      [](const std::string& arg) { return const_cast<char*>(arg.data()); });
  args.push_back(nullptr);
  int rc = execv(executable, args.data());
  PLOG_ERROR << "execv failed";
  return rc;
}
}
