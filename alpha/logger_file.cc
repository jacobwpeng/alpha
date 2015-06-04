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
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdio>
#include <cstring>

#include "compiler.h"
#include "logger.h"

namespace alpha {
    LoggerFile::LoggerFile(const std::string& path,
            const std::string& prog_name,
            const std::string& log_level_name)
        :path_(path),
        prog_name_(prog_name),
        log_level_name_(log_level_name) {
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

        char file[PATH_MAX];
        ::snprintf(file,
                sizeof(file),
                "%s/%s.%4d%02d%02d%02d.%s.log",
                path_.data(),
                prog_name_.data(),
                result.tm_year + 1900,
                result.tm_mon + 1,
                result.tm_mday,
                result.tm_hour,
                log_level_name_.data());
        fd_ = ::open(file, O_WRONLY | O_APPEND | O_CREAT, 0666);
        if (unlikely(fd_ == -1)) {
            perror("open");
        } else {
            UpdateSymLinkFile(strrchr(file, '/') + 1);
            file_create_time_ = now;
        }
    }

    void LoggerFile::UpdateSymLinkFile(const char* file) {
        static std::string symlink_path = SymLinkPath();
        unlink(symlink_path.data());
        symlink(file, symlink_path.data());
    }

    std::string LoggerFile::SymLinkPath() const {
        char file[PATH_MAX];
        ::snprintf(file, sizeof(file),
                "%s/%s.%s.log",
                path_.data(),
                prog_name_.data(),
                log_level_name_.data()
                );
        return file;
    }
}
