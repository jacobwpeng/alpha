/*
 * ==============================================================================
 *
 *       Filename:  Logger.h
 *        Created:  12/21/14 05:04:26
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#pragma once

#include <memory>
#include <vector>
#include <atomic>
#include <functional>
#include <alpha/LoggerFile.h>
#include <alpha/LoggerStream.h>

namespace alpha {
enum LogLevel {
  kLogLevelInfo = 0,
  kLogLevelWarning = 1,
  kLogLevelError = 2,
  kLogLevelFatal = 3
};
static const int kLogLevelNum = 4;

class Logger {
 private:
  struct LogVoidify {
    void operator&(std::ostream&) {}
  };

 public:
  using LoggerOutput = void (*)(LogLevel level, const char*, int);
  static void Init(const char* prog_name);
  static void set_logtostderr(bool enable);
  static void set_minloglevel(int level);
  static void set_logdir(alpha::Slice logdir);
  static void SendLog(LogLevel level, const char* buf, int len);
  static const char* GetLogLevelName(int level);
  static LogVoidify dummy_;

 private:
  Logger(LogLevel level, const LoggerOutput& output);
  static const char* LogLevelNames_[kLogLevelNum];
  static bool initialized_;
};

class LogEnv {
 public:
  static void Init();
  static bool logtostderr();
  static LogLevel minloglevel();
  static std::string logdir();

 private:
  friend class Logger;
  static bool logtostderr_;
  static LogLevel minloglevel_;
  static std::string logdir_;
};

class LogDestination {
 public:
  static void Init(const char* prog_name);
  static void SendLog(LogLevel level, const char* buf, int len);
  static void SendLogToStderr(LogLevel level, const char* buf, int len);
  static int GetLogNum(LogLevel level);

 private:
  using LogFiles = std::vector<std::unique_ptr<LoggerFile>>;
  using LogFilesPtr = std::unique_ptr<LogFiles>;
  static void AddFileSink(int level);
  static std::atomic_int logs_num_[kLogLevelNum];
  static const char* prog_name_;
  static LogFilesPtr files_;
};

class LogMessage final {
 public:
  LogMessage(const char* file,
             int line,
             LogLevel level,
             bool errno_message = false,
             const char* expr = nullptr);
  virtual ~LogMessage();
  alpha::LoggerStream& stream();

 private:
  void Flush();
  int preserved_errno() const;
  void LogStackTrace(LoggerStream& stream);

  bool flushed_;
  int preserved_errno_;
  int header_size_;
  const char* file_;
  int line_;
  LogLevel level_;
  bool errno_message_;
  const char* expr_;
  std::vector<char> log_buffer_;
  alpha::LoggerStream stream_;
};
}
#define ALPHA_TOSTRING1(x) #x
#define ALPHA_TOSTRING(x) ALPHA_TOSTRING1(x)
#define VodifyStream(stream) alpha::Logger::dummy_& stream

#define LOG_IF(level, cond) \
  !(cond)                   \
      ? (void)0             \
      : VodifyStream(alpha::LogMessage(__FILE__, __LINE__, level).stream())
#define LOG_INFO_IF(cond) LOG_IF(alpha::kLogLevelInfo, cond)
#define LOG_WARNING_IF(cond) LOG_IF(alpha::kLogLevelWarning, cond)
#define LOG_ERROR_IF(cond) LOG_IF(alpha::kLogLevelError, cond)
#define LOG_INFO LOG_INFO_IF(true)
#define LOG_WARNING LOG_WARNING_IF(true)
#define LOG_ERROR LOG_ERROR_IF(true)

#define PLOG_IF(level, cond) \
  !(cond) ? (void)0          \
          : VodifyStream(    \
                alpha::LogMessage(__FILE__, __LINE__, level, true).stream())
#define PLOG_INFO_IF(cond) PLOG_IF(alpha::kLogLevelInfo, cond)
#define PLOG_WARNING_IF(cond) PLOG_IF(alpha::kLogLevelWarning, cond)
#define PLOG_ERROR_IF(cond) PLOG_IF(alpha::kLogLevelError, cond)
#define PLOG_INFO PLOG_INFO_IF(true)
#define PLOG_WARNING PLOG_WARNING_IF(true)
#define PLOG_ERROR PLOG_ERROR_IF(true)

#define CHECK(expr)                                                        \
  (expr) ? (void)0 : VodifyStream(alpha::LogMessage(__FILE__,              \
                                                    __LINE__,              \
                                                    alpha::kLogLevelFatal, \
                                                    false,                 \
                                                    ALPHA_TOSTRING(expr))  \
                                      .stream())
#define PCHECK(expr)                                                       \
  (expr) ? (void)0 : VodifyStream(alpha::LogMessage(__FILE__,              \
                                                    __LINE__,              \
                                                    alpha::kLogLevelFatal, \
                                                    true,                  \
                                                    ALPHA_TOSTRING(expr))  \
                                      .stream())

#ifdef NDEBUG
#define DLOG_INFO_IF(cond) LOG_INFO_IF(false)
#define DLOG_WARNING_IF(cond) LOG_WARNING_IF(false)
#define DLOG_ERROR_IF(cond) LOG_ERROR_IF(false)
#define DLOG_INFO DLOG_INFO_IF(false)
#define DLOG_WARNING DLOG_WARNING_IF(false)
#define DLOG_ERROR DLOG_ERROR_IF(false)
#define DCHECK(expr) alpha::NullStream()
#else
#define DLOG_INFO_IF LOG_INFO_IF
#define DLOG_WARNING_IF LOG_WARNING_IF
#define DLOG_ERROR_IF LOG_ERROR_IF
#define DLOG_INFO LOG_INFO
#define DLOG_WARNING LOG_WARNING
#define DLOG_ERROR LOG_ERROR
#define DCHECK(expr) CHECK(expr)
#endif

#ifndef ALPHA_STRIP_LOG
#define ALPHA_STRIP_LOG 0
#endif

#if ALPHA_STRIP_LOG >= 1
#undef LOG_INFO_IF
#undef PLOG_INFO_IF
#define LOG_INFO_IF(cond) alpha::NullStream()
#define PLOG_INFO_IF(cond) alpha::NullStream()
#endif

#if ALPHA_STRIP_LOG >= 2
#undef LOG_WARNING_IF
#undef PLOG_WARNING_IF
#define LOG_WARNING_IF(cond) alpha::NullStream()
#define PLOG_WARNING_IF(cond) alpha::NullStream()
#endif

#if ALPHA_STRIP_LOG >= 3
#undef LOG_ERROR_IF
#undef PLOG_ERROR_IF
#define LOG_ERROR_IF(cond) alpha::NullStream()
#define PLOG_ERROR_IF(cond) alpha::NullStream()
#endif

