/*
 * ==============================================================================
 *
 *       Filename:  logger.cc
 *        Created:  12/21/14 05:17:53
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#include "logger.h"

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <functional>
#include <alpha/logger_file.h>
#include <alpha/compiler.h>

namespace alpha {
Logger::LogVoidify Logger::dummy_;
const char* Logger::LogLevelNames_[kLogLevelNum] = {"INFO", "WARN", "ERROR",
                                                    "FATAL"};
bool Logger::initialized_ = false;

bool LogEnv::logtostderr_ = false;
LogLevel LogEnv::minloglevel_ = kLogLevelInfo;
std::string LogEnv::logdir_ = "/tmp";

std::atomic_int LogDestination::logs_num_[alpha::kLogLevelNum];
const char* LogDestination::prog_name_ = nullptr;
LogDestination::LogFilesPtr LogDestination::files_;

static const int kLogBufferSize = 30 * 1024;

static const char* const_basename(const char* name) {
  auto p = ::strrchr(name, '/');
  return p ? p + 1 : name;
}

void Logger::Init(const char* prog_name) {
  LogEnv::Init();
  LogDestination::Init(const_basename(prog_name));
  initialized_ = true;
}

const char* Logger::GetLogLevelName(int level) {
  assert(level < kLogLevelNum);
  return LogLevelNames_[level];
}

void Logger::SendLog(LogLevel level, const char* buf, int len) {
  assert(len >= 0);
  static bool first_log_before_init = true;
  if (!initialized_ && first_log_before_init) {
    alpha::Slice warning("[WARN]Log before init will go to stderr.\n");
    LogDestination::SendLogToStderr(kLogLevelWarning, warning.data(),
                                    warning.size());
    first_log_before_init = false;
  }
  if (LogEnv::logtostderr() || !initialized_) {
    LogDestination::SendLogToStderr(level, buf, len);
  } else {
    if (level == kLogLevelFatal) {
      LogDestination::SendLogToStderr(level, buf, len);
    }
    LogDestination::SendLog(level, buf, len);
  }
}

void LogEnv::Init() {
  const char* logdir = getenv("alpha_logdir");
  const char* logtostderr = getenv("alpha_logtostderr");
  const char* minloglevel = getenv("alpha_minloglevel");

  if (logdir) {
    logdir_ = logdir;
    if (!logdir_.empty() && *logdir_.rbegin() == '/') {
      logdir_ = logdir_.substr(0, logdir_.size() - 1);
    }
  }
  if (logtostderr && logtostderr[0] == '1') {
    logtostderr_ = true;
  }
  if (minloglevel) {
    auto level = minloglevel[0] - '0';
    if (level >= 0 && level <= 3) {
      minloglevel_ = static_cast<LogLevel>(level);
    }
  }
}

bool LogEnv::logtostderr() { return logtostderr_; }
LogLevel LogEnv::minloglevel() { return minloglevel_; }
std::string LogEnv::logdir() { return logdir_; }

void LogDestination::Init(const char* prog_name) {
  prog_name_ = prog_name;
  if (!LogEnv::logtostderr()) {
    const int minloglevel = LogEnv::minloglevel();
    files_.reset(new LogDestination::LogFiles());
    for (int i = 0; i < kLogLevelNum; ++i) {
      if (i >= minloglevel) {
        AddFileSink(i);
      } else {
        files_->push_back(nullptr);
      }
    }
  }
}

void LogDestination::SendLog(LogLevel level, const char* buf, int len) {
  const int log_level = level;
  const int minloglevel = LogEnv::minloglevel();
  if (log_level >= minloglevel) {
    ++logs_num_[level];
    for (int i = minloglevel; i <= log_level; ++i) {
      files_->at(i)->Write(buf, len);
    }
  }
}

void LogDestination::SendLogToStderr(LogLevel level, const char* buf, int len) {
  logs_num_[level]++;
  ::write(STDERR_FILENO, buf, len);
}

int LogDestination::GetLogNum(LogLevel level) { return logs_num_[level]; }

void LogDestination::AddFileSink(int level) {
  files_->push_back(std::unique_ptr<LoggerFile>(new LoggerFile(
      LogEnv::logdir(), prog_name_, Logger::GetLogLevelName(level))));
}

LogMessage::LogMessage(const char* file, int line, LogLevel level,
                       bool errno_message, const char* expr)
    : flushed_(false),
      file_(file),
      line_(line),
      level_(level),
      errno_message_(errno_message),
      expr_(expr),
      log_buffer_(kLogBufferSize) {
  static const int kTimeFormatBufferSize = 256;
  static thread_local struct tm tm;
  static thread_local time_t last_second = 0;
  static thread_local char time_format[kTimeFormatBufferSize];
  static thread_local pid_t tid = 0;

  preserved_errno_ = errno;
  struct timeval tv;
  int ret = gettimeofday(&tv, NULL);
  assert(ret == 0);
  (void)ret;
  if (unlikely(tid == 0)) {
    tid = syscall(SYS_gettid);
  }
  if (tv.tv_sec != last_second) {
    last_second = tv.tv_sec;
    if (NULL != localtime_r(&tv.tv_sec, &tm)) {
      strftime(time_format, sizeof(time_format),
               "[%Y-%m-%d %H:%M:%S.%%06u %%d %%s %%s:%%d] ", &tm);
    }
  }
  auto nbytes =
      snprintf(log_buffer_.data(), kLogBufferSize, time_format, tv.tv_usec, tid,
               Logger::GetLogLevelName(level), const_basename(file_), line_);
  assert(nbytes > 0 && nbytes < kLogBufferSize);
  stream_.rdbuf()->pubsetbuf(log_buffer_.data() + nbytes,
                             kLogBufferSize - nbytes - 1);
  header_size_ = nbytes;
}

LogMessage::~LogMessage() {
  if (errno_message_) {
    char message[256];
    char* m = strerror_r(preserved_errno(), message, sizeof(message));
    stream() << ": " << m;
  }
  Flush();
  errno = preserved_errno_;
  if (unlikely(level_ == kLogLevelFatal)) {
    abort();
  }
}

alpha::LoggerStream& LogMessage::stream() { return stream_; }

int LogMessage::preserved_errno() const { return preserved_errno_; }

void LogMessage::Flush() {
  if (flushed_) {
    return;
  }
  int len = header_size_ + stream_.streambuf()->used();
  assert(len < kLogBufferSize);
  if (log_buffer_[len - 1] != '\n') {
    log_buffer_[len] = '\n';
    len += 1;
  }
  Logger::SendLog(level_, log_buffer_.data(), len);
  flushed_ = true;
}
}
