/*
 * =============================================================================
 *
 *       Filename:  ProcessLockFile.h
 *        Created:  05/15/15 15:19:33
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include <string>
#include <alpha/Slice.h>

namespace alpha {
class ProcessLockFile {
 public:
  ProcessLockFile(Slice file);
  ~ProcessLockFile();

 private:
  int fd_ = -1;
  std::string file_;
};
}

