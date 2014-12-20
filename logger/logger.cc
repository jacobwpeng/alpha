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

#include <cassert>

namespace alpha {
    Logger::Voidify Logger::dummy_;
    Logger* Logger::instance_ = nullptr;
    const char* Logger::prog_name_ = nullptr;
    const char* Logger::LogLevelNames_[Logger::LogLevelNum_];
    const char* Logger::LogLevelBlankSpace_[Logger::LogLevelNum_];
    Logger::Logger(LogLevel level, const Logger::LoggerOutput& output)
        :log_level_(level), logger_output_(output) {
        }

    void Logger::Init(const char* prog_name, Logger::LoggerOutput output) {
        static Logger logger(LogLevel::Debug, output);
        instance_ = &logger;
        prog_name_ = prog_name;
        LogLevelNames_[0] = "ERROR";
        LogLevelNames_[1] = "WARNING";
        LogLevelNames_[2] = "INFO";
        LogLevelNames_[3] = "DEBUG";
        LogLevelBlankSpace_[0] = "     ";
        LogLevelBlankSpace_[1] = "   ";
        LogLevelBlankSpace_[2] = "      ";
        LogLevelBlankSpace_[3] = "     ";
    }

    const char* Logger::GetLogLevelName(int level) {
        assert (level <= LogLevelNum_);
        return LogLevelNames_[level];
    }

    const char* Logger::GetLogLevelBlankSpaceNum(int level) {
        assert (level <= LogLevelNum_);
        return LogLevelBlankSpace_[level];
    }

    void Logger::Append(LogLevel level, const char* content, int len) {
        assert (len >= 0);
        assert (logger_output_);
        logger_output_(level, content, len);
    }

}
