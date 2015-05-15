/*
 * =============================================================================
 *
 *       Filename:  process_lock_file.cc
 *        Created:  05/15/15 15:20:32
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * =============================================================================
 */

#include "process_lock_file.h"

#include <unistd.h>
#include <sys/file.h>
#include "logger.h"

namespace alpha {
    ProcessLockFile::ProcessLockFile(Slice file)
        :file_(file.ToString()) {
        fd_ = ::open(file.data(), O_CREAT | O_WRONLY, 0666);
        // PCHECK(fd_ != -1) << "open failed, file = " << file.ToString();
        if (fd_ == -1) {
            PLOG_ERROR << "open failed, file = " << file.ToString();
            abort();
        }
        int err = ::flock(fd_, LOCK_EX | LOCK_NB);
        if (err) {
            PLOG_ERROR << "flock failed";
            abort();
        }
        std::string pid = std::to_string(getpid());
        err = ::write(fd_, pid.data(), pid.size());
        //PCHECK(err != -1) << "write failed";
        if (err == -1) {
            PLOG_ERROR << "write failed, file = " << file.ToString();
            abort();
        }
    }

    ProcessLockFile::~ProcessLockFile() {
        if (fd_ != -1) {
            ::close(fd_);
        }
    }
}
