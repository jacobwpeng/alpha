/*
 * ==============================================================================
 *
 *       Filename:  LoggerFile.h
 *        Created:  03/29/15 17:39:54
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#include "LoggerFile.h"
#include <limits.h>
#include <cstdio>
#include <cstring>

#include "compiler.h"
#include "logger.h"

namespace alpha {
LoggerFile::LoggerFile(const std::string& path, const std::string& prog_name,
                       const std::string& log_level_name)
    : path_(path), prog_name_(prog_name), log_level_name_(log_level_name) {}

void LoggerFile::Write(const char* content, int len) {
  MaybeChangeLogFile();
  if (unlikely(!file_)) {
    ::fprintf(stderr, content, len);
  } else {
    int n = file_.Write(content, len);
    if (unlikely(n != len)) {
      ::perror("write");
    }
  }
}

void LoggerFile::MaybeChangeLogFile() {
  TimeStamp now = Now();
  if (likely(InSameHour(now, file_create_time_) && file_)) {
    return;
  }

  if (likely(file_.Valid())) {
    bool ok = file_.Close();
    assert(ok);
    (void)ok;
  }

  struct tm result;
  const time_t t = now / kMilliSecondsPerSecond;
  if (unlikely(nullptr == ::localtime_r(&t, &result))) {
    perror("localtime_r");
    return;
  }

  char path[PATH_MAX];
  ::snprintf(path, sizeof(path), "%s/%s.%4d%02d%02d%02d.%s.log", path_.c_str(),
             prog_name_.c_str(), result.tm_year + 1900, result.tm_mon + 1,
             result.tm_mday, result.tm_hour, log_level_name_.c_str());
  int open_flags = O_WRONLY | O_APPEND | O_CREAT;
  File new_file(path, open_flags);
  if (!new_file) {
    perror("open");
  } else {
    file_ = std::move(new_file);
    UpdateSymLinkFile(strrchr(path, '/') + 1);
    file_create_time_ = now;
  }
}

void LoggerFile::UpdateSymLinkFile(const char* file) {
  std::string symlink_path = SymLinkPath();
  unlink(symlink_path.c_str());
  symlink(file, symlink_path.c_str());
}

std::string LoggerFile::SymLinkPath() const {
  char path[PATH_MAX];
  ::snprintf(path, sizeof(path), "%s/%s.%s.log", path_.c_str(),
             prog_name_.c_str(), log_level_name_.c_str());
  return path;
}
}
