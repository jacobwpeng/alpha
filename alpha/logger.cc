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
#include "logger_file.h"

#include <unistd.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <functional>

namespace alpha {
    Logger::LogVoidify Logger::dummy_;
    const char* Logger::LogLevelNames_[kLogLevelNum] = {
        "INFO", "WARN", "ERROR"
    };
    bool Logger::initialized_ = false;

    bool LogEnv::logtostderr_ = false;
    LogLevel LogEnv::minloglevel_ = kLogLevelInfo;
    std::string LogEnv::logdir_ = "/tmp";

    const char* LogDestination::prog_name_ = nullptr;
    LogDestination::LogFilesPtr LogDestination::files_;

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
        assert (level < kLogLevelNum);
        return LogLevelNames_[level];
    }

    void Logger::SendLog(LogLevel level, const char* buf, int len) {
        assert (len >= 0);
        static bool first_log_before_init = true;
        if (!initialized_ && first_log_before_init) {
            alpha::Slice warning("[WARN]Log before init will go to stderr.\n");
            LogDestination::SendLogToStderr(kLogLevelWarning,
                    warning.data(), warning.size());
            first_log_before_init = false;
        }
        if (LogEnv::logtostderr() || !initialized_) {
            LogDestination::SendLogToStderr(level, buf, len);
        } else {
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

    bool LogEnv::logtostderr() {
        return logtostderr_;
    }
    LogLevel LogEnv::minloglevel() {
        return minloglevel_;
    }
    std::string LogEnv::logdir() {
        return logdir_;
    }

    void LogDestination::Init(const char* prog_name) {
        prog_name_ = prog_name;
        if (!LogEnv::logtostderr()) {
            const int minloglevel = LogEnv::minloglevel();
            files_.reset (new LogDestination::LogFiles());
            for (int i = 0;
                    i < kLogLevelNum;
                    ++i) {
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
            for (int i = minloglevel; i <= log_level; ++i) {
                files_->at(i)->Write(buf, len);
            }
        }
    }

    void LogDestination::SendLogToStderr(LogLevel level, const char* buf,
            int len) {
        ::write(STDERR_FILENO, buf, len);
    }

    void LogDestination::AddFileSink(int level) {
        files_->push_back(std::unique_ptr<LoggerFile>(
                        new LoggerFile(
                            LogEnv::logdir(),
                            prog_name_,
                            Logger::GetLogLevelName(level)
                        )
                    )
                );
    }
}
