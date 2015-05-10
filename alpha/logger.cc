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

#include <cassert>
#include <cstdio>
#include <cstring>
#include <functional>

namespace alpha {
    static const char* kDefaultLogDir = "/tmp";
    static const LogLevel kDefaultLogLevel = LogLevel::Info;
    Logger::Voidify Logger::dummy_;
    Logger* Logger::instance_ = nullptr;
    const char* Logger::prog_name_ = nullptr;
    const char* Logger::LogLevelNames_[Logger::LogLevelNum_];

    void Logger::LogToStderr(LogLevel level, const char* content, int len) {
        ::fwrite(content, 1, len, stderr);
    }

    Logger::Logger(LogLevel level, const Logger::LoggerOutput& output)
        :log_level_(level), logger_output_(output) {
        }

    void Logger::Init(const char* prog_name) {
       const char* log_dir = secure_getenv("alpha_logdir");
       const char* logtostderr_env = secure_getenv("alpha_logtostderr");
       LogLevel min_log_level = kDefaultLogLevel;
       const char* min_log_level_env = secure_getenv("alpha_minloglevel");
       Logger::LoggerOutput output;
       if (log_dir == nullptr) {
           log_dir = kDefaultLogDir;
       }
       if (min_log_level_env != nullptr) {
           std::string min_level(min_log_level_env);
           if (min_level == "0") {
               min_log_level = LogLevel::Fatal;
           } else if (min_level == "1") {
               min_log_level = LogLevel::Error;
           } else if (min_level == "2") {
               min_log_level = LogLevel::Warning;
           } else if (min_level == "3") {
               min_log_level = LogLevel::Info;
           }
       }
       if (logtostderr_env != nullptr && ::strcmp("1", logtostderr_env) == 0) {
           output = &Logger::LogToStderr;
       } else {
           std::string basename(prog_name);
           auto pos = basename.rfind("/");
           if (pos != std::string::npos) {
               basename = basename.substr(pos + 1);
           }
           static LoggerFile file(log_dir, basename);
           using namespace std::placeholders;
           output = std::bind(&LoggerFile::Write, &file, _1, _2, _3);
       }
        static Logger logger(min_log_level, output);
        instance_ = &logger;
        prog_name_ = prog_name;
        LogLevelNames_[0] = "FATAL";
        LogLevelNames_[1] = "ERROR";
        LogLevelNames_[2] = "WARN";
        LogLevelNames_[3] = "INFO";
    }

    const char* Logger::GetLogLevelName(int level) {
        assert (level <= LogLevelNum_);
        return LogLevelNames_[level];
    }

    void Logger::Append(LogLevel level, const char* content, int len) {
        assert (len >= 0);
        static bool first_log_before_init = true;
        auto output = logger_output_ == nullptr ? LogToStderr : logger_output_;
        if (logger_output_ == nullptr && first_log_before_init) {
            ::fputs("Log before Init will go to stderr\n", stderr);
            first_log_before_init = true;
        }
        assert (logger_output_);
        logger_output_(level, content, len);
    }

}
