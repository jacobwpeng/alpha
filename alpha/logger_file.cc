/*
 * ==============================================================================
 *
 *       Filename:  logger_file.cc
 *        Created:  03/29/15 17:39:54
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#include "logger_file.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdio>

#include "compiler.h"
#include "logger.h"

namespace alpha {
    LoggerFile::LoggerFile(Slice path, Slice log_level_name, Slice prog_name)
        :file_create_time_(0),
         path_(path.ToString()),
         log_level_name_(log_level_name.ToString()),
         prog_name_(prog_name.ToString()) {
    }

    LoggerFile::~LoggerFile() {
        if (fd_ != -1) {
            ::close(fd_);
        }
    }

    void LoggerFile::Write(const char* content, int len) {
        MaybeChangeLogFile();
        if (unlikely(fd_ == -1)) {
            ::fprintf(stderr, content, len);
        } else {
            ssize_t n = ::write(fd_, content, len);
            if (unlikely(n != len)) {
                ::perror("write trucated");
            }
        }
    }

    void LoggerFile::MaybeChangeLogFile() {
        TimeStamp now = Now();
        if (likely(InSameHour(now, file_create_time_) && fd_ != -1)) {
            return;
        }

        if (likely(fd_ != -1)) {
            ::close(fd_);
        }

        struct tm result;
        const time_t t = now / kMilliSecondsPerSecond;
        if (unlikely(nullptr == ::localtime_r(&t, &result))) {
            perror("localtime_r");
            return;
        }

        char file_name[256];
        ::snprintf(file_name, sizeof(file_name), "%s/%s-%4d%02d%02d%02d.%s.log", 
                path_.c_str(),
                prog_name_.c_str(),
                result.tm_year + 1900,
                result.tm_mon + 1,
                result.tm_mday,
                result.tm_hour,
                log_level_name_.c_str()
                );
        fd_ = ::open(file_name, O_WRONLY | O_APPEND | O_CREAT, 0666);
        if (unlikely(fd_ == -1)) {
            perror("open");
        } else {
            file_create_time_ = now;
        }
    }
}
