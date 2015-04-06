/*
 * ==============================================================================
 *
 *       Filename:  logger.h
 *        Created:  12/21/14 05:04:26
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#ifndef  __LOGGER_H__
#define  __LOGGER_H__

#include <functional>
#include "logger_formatter.h"

namespace alpha {
    enum class LogLevel : short {
        Error = 0,
        Warning = 1,
        Info = 2,
        Debug = 3,
        TotalNum = 4,
    };

    struct BasenameRetriever {
        template<int len>
        BasenameRetriever(const char (&arr)[len])
        :basename(NULL) {
            int pos = len - 1;                      /* skil '\0' */
            while (pos >= 0) {
                if (arr[pos] == '/') {
                    basename = arr + pos + 1;
                    break;
                }
                else {
                    --pos;
                }
            }
            if (basename == NULL) basename = arr;
        }
        const char* basename;
    };

    class Logger {
        private:
            struct Voidify {
                void operator&(std::ostream&) {
                }
            };

        public:
            typedef std::function<void (LogLevel level, 
                    const char* content, int len)> LoggerOutput;

            static void Init(const char* prog_name,
                    LoggerOutput output = nullptr);
            static void LogToStderr(LogLevel level, const char* content, int len);

            LogLevel GetLogLevel() const { return log_level_; }
            void Append(LogLevel level, const char* content, int len);
            static const char* GetLogLevelName(int level);
            static Logger* Instance() { return instance_; }

            static Voidify dummy_;
        private:
            Logger(LogLevel level, const LoggerOutput& output);
            LogLevel log_level_;
            LoggerOutput logger_output_;
            static const char* prog_name_;
            static const int LogLevelNum_ = 4;
            static const char* LogLevelNames_[LogLevelNum_];
            static Logger* instance_;
    };
}

#define LOG_COND_IF_IMPL(level, cond, errno_message) \
    (alpha::Logger::Instance()->GetLogLevel() < level || (!cond)) ? (void)0 : \
        alpha::Logger::dummy_ & (alpha::LoggerFormatter(\
                    alpha::Logger::Instance(), \
                        alpha::BasenameRetriever(__FILE__).basename, \
                        __FUNCTION__, __LINE__, static_cast<int>(level), errno_message \
                    ).stream())


#define LOG_COND_IF(level, cond) LOG_COND_IF_IMPL(level, cond, false)
#define PLOG_COND_IF(level, cond) LOG_COND_IF_IMPL(level, cond, true)

#define LOG_LEVEL_IF(level) LOG_COND_IF(level, true)
#define PLOG_LEVEL_IF(level) PLOG_COND_IF(level, true)

#define LOG_DEBUG LOG_LEVEL_IF(alpha::LogLevel::Debug)
#define LOG_INFO LOG_LEVEL_IF(alpha::LogLevel::Info)
#define LOG_WARNING LOG_LEVEL_IF(alpha::LogLevel::Warning)
#define LOG_ERROR LOG_LEVEL_IF(alpha::LogLevel::Error)

#define PLOG_DEBUG PLOG_LEVEL_IF(alpha::LogLevel::Debug)
#define PLOG_INFO PLOG_LEVEL_IF(alpha::LogLevel::Info)
#define PLOG_WARNING PLOG_LEVEL_IF(alpha::LogLevel::Warning)
#define PLOG_ERROR PLOG_LEVEL_IF(alpha::LogLevel::Error)

#define LOG_DEBUG_IF(cond) LOG_COND_IF(alpha::LogLevel::Debug, (cond))
#define LOG_INFO_IF(cond) LOG_COND_IF(alpha::LogLevel::Info, (cond))
#define LOG_WARNING_IF(cond) LOG_COND_IF(alpha::LogLevel::Warning, (cond))
#define LOG_ERROR_IF(cond) LOG_COND_IF(alpha::LogLevel::Error, (cond))

#define PLOG_DEBUG_IF(cond) PLOG_COND_IF(alpha::LogLevel::Debug, (cond))
#define PLOG_INFO_IF(cond) PLOG_COND_IF(alpha::LogLevel::Info, (cond))
#define PLOG_WARNING_IF(cond) PLOG_COND_IF(alpha::LogLevel::Warning, (cond))
#define PLOG_ERROR_IF(cond) PLOG_COND_IF(alpha::LogLevel::Error, (cond))

#ifdef NDEBUG
#define DLOG_DEBUG_IF(cond) LOG_DEBUG_IF(false)
#define DLOG_INFO_IF(cond) LOG_INFO_IF(false)
#define DLOG_WARNING_IF(cond) LOG_WARNING_IF(false)
#define DLOG_ERROR_IF(cond) LOG_ERROR_IF(false)
#define DLOG_DEBUG DLOG_DEBUG_IF(false)
#define DLOG_INFO DLOG_INFO_IF(false)
#define DLOG_WARNING DLOG_WARNING_IF(false)
#define DLOG_ERROR DLOG_ERROR_IF(false)
#else
#define DLOG_DEBUG_IF LOG_DEBUG_IF
#define DLOG_INFO_IF LOG_INFO_IF
#define DLOG_WARNING_IF LOG_WARNING_IF
#define DLOG_ERROR_IF LOG_ERROR_IF
#define DLOG_DEBUG LOG_DEBUG
#define DLOG_INFO LOG_INFO
#define DLOG_WARNING LOG_WARNING
#define DLOG_ERROR LOG_ERROR
#endif

#endif   /* ----- #ifndef __LOGGER_H__  ----- */
