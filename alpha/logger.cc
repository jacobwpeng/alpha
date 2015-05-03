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
#include <functional>

namespace alpha {
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

    void Logger::Init(const char* prog_name, Logger::LoggerOutput output
            , LogLevel level) {
        if (output == nullptr) {
            std::string basename(prog_name);
            auto pos = basename.rfind("/");
            if (pos != std::string::npos) {
                basename = basename.substr(pos + 1);
            }
            static LoggerFile file("/tmp", basename);
            using namespace std::placeholders;
            output = std::bind(&LoggerFile::Write, &file, _1, _2, _3);
        }
        static Logger logger(level, output);
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
        assert (logger_output_);
        logger_output_(level, content, len);
    }

}
