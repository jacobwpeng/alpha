/*
 * ==============================================================================
 *
 *       Filename:  log_file.cc
 *        Created:  12/21/14 06:56:00
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#include "log_file.h"

#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

namespace alpha {
    LogFile::LogFile(const char* file)
    :fd_(-1), file_(file) {
    }

    LogFile::~LogFile() {
        if (fd_ != -1) {
            ::close(fd_);
        }
    }

    void LogFile::Append(const char* content, int len) {
        if (MaybeCreateFile()) {
            ::write(fd_, content, len);
        }
    }

    bool LogFile::MaybeCreateFile() {
        if (likely(fd_ != -1)) {
            return true;
        }

        int fd = ::open(file_, O_APPEND | O_CREAT | O_WRONLY, 0644);
        if (fd == -1) {
            ::perror("open");
            return false;
        } else {
            fd_ = fd;
            return true;
        }
    }
}
