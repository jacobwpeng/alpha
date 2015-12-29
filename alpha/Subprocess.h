/*
 * =============================================================================
 *
 *       Filename:  Subprocess.h
 *        Created:  12/10/15 10:51:54
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include <unistd.h>
#include <map>
#include <vector>
#include <string>
#include <boost/operators.hpp>
#include <alpha/slice.h>

namespace alpha {
class SubprocessError : public std::exception {
 public:
  SubprocessError(alpha::Slice what) : what_(what.ToString()) {}
  virtual ~SubprocessError() = default;
  virtual const char* what() const throw() override { return what_.c_str(); }

 private:
  std::string what_;
};

class SubprocessSpawnError final : public SubprocessError {
 public:
  SubprocessSpawnError(alpha::Slice what, int error)
      : SubprocessError(what), error_(error) {}

 private:
  int error_;
};

class ProcessReturnCode {
 public:
  enum class State : uint8_t {
    kNotStarted = 0,
    kRunning = 1,
    kExited = 2,
    kKilledBySignal = 3
  };
  ProcessReturnCode() : raw_status_(kRawStatusNotStarted) {}
  ProcessReturnCode(int raw_status) : raw_status_(raw_status) {}

  void SetRunning() { raw_status_ = kRawStatusRunning; }

  State state() const;
  std::string status() const;
  bool NotStarted() const;
  bool Running() const;
  bool TerminatedNormally() const;
  int ExitCode() const;  // Valid only if TerminatedNormally
  bool KilledBySignal() const;
  int SignalReceived() const;  // Valid only if KilledBySignal

 private:
  static const int kRawStatusRunning = -1;
  static const int kRawStatusNotStarted = -2;
  int raw_status_;
};

class Subprocess {
 public:
  class Options : boost::orable<Options> {
   private:
    friend class Subprocess;

   public:
    static const int kActionClose = -1;
    Options();
    Options& fd(int fd, int action);
    Options& CloseOtherFds();
    Options& operator|=(const Options& other);

   private:
    bool close_other_fds_{false};
    std::map<int, int> fd_actions_;
  };
  Subprocess(const std::vector<std::string>& argv,
             const char* executable = nullptr,
             const Options& options = Options());

  ProcessReturnCode Wait();
  ProcessReturnCode Poll();
  pid_t pid() const { return pid_; }

 private:
  pid_t pid_;
  ProcessReturnCode return_code_;
};
}
