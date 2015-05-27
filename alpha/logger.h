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

#include <memory>
#include <vector>
#include <atomic>
#include <functional>
#include "slice.h"
#include "logger_formatter.h"
#include "logger_file.h"

namespace alpha {
    enum LogLevel {
        kLogLevelInfo = 0,
        kLogLevelWarning = 1,
        kLogLevelError = 2
    };

    static const int kLogLevelNum = 3;

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
            struct LogVoidify {
                void operator&(std::ostream&) {
                }
            };

        public:
            using LoggerOutput = void(*)(LogLevel level, const char*, int);

            static void Init(const char* prog_name);
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
}

#define LOG_COND_IF_IMPL(level, cond, errno_message)                    \
    (level < alpha::LogEnv::minloglevel() || (!cond)) ?                 \
    (void)0 :                                                           \
    alpha::Logger::dummy_ & (alpha::LoggerFormatter(                    \
        alpha::BasenameRetriever(__FILE__).basename,                    \
        __FUNCTION__,                                                   \
        __LINE__,                                                       \
        static_cast<int>(level),                                        \
        errno_message                                                   \
    ).stream())


#define LOG_COND_IF(level, cond) LOG_COND_IF_IMPL(level, cond, false)
#define PLOG_COND_IF(level, cond) LOG_COND_IF_IMPL(level, cond, true)

#define LOG_LEVEL_IF(level) LOG_COND_IF(level, true)
#define PLOG_LEVEL_IF(level) PLOG_COND_IF(level, true)

#define LOG_INFO LOG_LEVEL_IF(alpha::kLogLevelInfo)
#define LOG_WARNING LOG_LEVEL_IF(alpha::kLogLevelWarning)
#define LOG_ERROR LOG_LEVEL_IF(alpha::kLogLevelError)

#define PLOG_INFO PLOG_LEVEL_IF(alpha::kLogLevelInfo)
#define PLOG_WARNING PLOG_LEVEL_IF(alpha::kLogLevelWarning)
#define PLOG_ERROR PLOG_LEVEL_IF(alpha::kLogLevelError)

#define LOG_INFO_IF(cond) LOG_COND_IF(alpha::kLogLevelInfo, (cond))
#define LOG_WARNING_IF(cond) LOG_COND_IF(alpha::kLogLevelWarning, (cond))
#define LOG_ERROR_IF(cond) LOG_COND_IF(alpha::kLogLevelError, (cond))

#define PLOG_INFO_IF(cond) PLOG_COND_IF(alpha::kLogLevelInfo, (cond))
#define PLOG_WARNING_IF(cond) PLOG_COND_IF(alpha::kLogLevelWarning, (cond))
#define PLOG_ERROR_IF(cond) PLOG_COND_IF(alpha::kLogLevelError, (cond))

#ifdef NDEBUG
#define DLOG_INFO_IF(cond) LOG_INFO_IF(false)
#define DLOG_WARNING_IF(cond) LOG_WARNING_IF(false)
#define DLOG_ERROR_IF(cond) LOG_ERROR_IF(false)
#define DLOG_INFO DLOG_INFO_IF(false)
#define DLOG_WARNING DLOG_WARNING_IF(false)
#define DLOG_ERROR DLOG_ERROR_IF(false)
#else
#define DLOG_INFO_IF LOG_INFO_IF
#define DLOG_WARNING_IF LOG_WARNING_IF
#define DLOG_ERROR_IF LOG_ERROR_IF
#define DLOG_INFO LOG_INFO
#define DLOG_WARNING LOG_WARNING
#define DLOG_ERROR LOG_ERROR
#endif

#endif   /* ----- #ifndef __LOGGER_H__  ----- */
