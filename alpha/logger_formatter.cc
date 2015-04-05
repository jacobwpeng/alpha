/*
 * ==============================================================================
 *
 *       Filename:  logger_formatter.cc
 *        Created:  12/20/14 05:46:39
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#include "logger_formatter.h"

#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <cstdio>
#include <cassert>

#include "compiler.h"
#include "logger.h"

namespace alpha {

    namespace detail {
        static __thread struct tm tm;
        static __thread time_t lasttime = 0;
        static __thread char time_fmt[256];
        static __thread pid_t tid = 0;
    }

    LoggerFormatter::LoggerFormatter(Logger* logger, const char* basename, 
            const char* funcname, int lineno, int level, bool needs_errno_message)
    :logger_(logger), log_level_(level), needs_errno_message_(needs_errno_message) {

        header_len_ = FormatHeader(basename, funcname, lineno, level);
        stream_.rdbuf()->pubsetbuf(buf + header_len_, 
                LoggerFormatter::kMaxLogLength - header_len_ - 1);
    }

    LoggerFormatter::~LoggerFormatter() {
        if (needs_errno_message_) {
            char errno_message[256];
            char* m = strerror_r(errno, errno_message, sizeof(errno_message));
            stream_ << ": " << m;
        }
        int len = header_len_ + stream_.streambuf()->used();
        buf[len] = '\n';
        len += 1;
        logger_->Append(static_cast<LogLevel>(log_level_), buf, len);
    }

    int LoggerFormatter::FormatHeader(const char* basename, const char* funcname,
            int lineno, int level) {

        struct timeval tv;
        int ret = gettimeofday(&tv, NULL);
        assert (ret == 0);
        (void)ret;
        if (unlikely(detail::tid == 0)) {
            detail::tid = syscall(SYS_gettid);
        }
        if (tv.tv_sec != detail::lasttime) {
            detail::lasttime = tv.tv_sec;
            if (NULL != localtime_r(&tv.tv_sec, &detail::tm)) {
                strftime(detail::time_fmt, sizeof detail::time_fmt, 
                    "[%Y-%m-%d %H:%M:%S.%%06u %%s:%%d %%s %%5.d][%%s] ", 
                    &detail::tm);
            }
        }

        return snprintf(buf, LoggerFormatter::kMaxLogLength, detail::time_fmt, 
                tv.tv_usec, basename, lineno, funcname, detail::tid, 
                Logger::GetLogLevelName(level));
    }
}
