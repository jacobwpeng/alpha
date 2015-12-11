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

#ifndef __ALPHA_SUBPROCESS_H__
#define __ALPHA_SUBPROCESS_H__

#include <unistd.h>
#include <vector>
#include <string>
#include "slice.h"

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

class SubprocessReturnCode {
 public:
  enum class State : uint8_t {
    kNotStarted = 0,
    kRunning = 1,
    kExited = 2,
    kKilledBySignal = 3
  };
  SubprocessReturnCode() : raw_status_(kRawStatusNotStarted) {}
  SubprocessReturnCode(int raw_status) : raw_status_(raw_status) {}

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
  Subprocess(const std::vector<std::string>& argv,
             const char* executable = nullptr);

  SubprocessReturnCode Wait();
  SubprocessReturnCode Poll();
  pid_t pid() const { return pid_; }

 private:
  pid_t pid_;
  SubprocessReturnCode return_code_;
};
}

#endif /* ----- #ifndef __ALPHA_SUBPROCESS_H__  ----- */
