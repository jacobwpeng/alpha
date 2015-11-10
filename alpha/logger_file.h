/*
 * ==============================================================================
 *
 *       Filename:  logger_file.h
 *        Created:  03/27/15 21:27:20
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#ifndef __LOGGER_FILE_H__
#define __LOGGER_FILE_H__

#include <string>
#include "slice.h"
#include "time_util.h"

namespace alpha {
class Slice;
class LoggerFile {
 public:
  LoggerFile(const std::string& path, const std::string& prog_name,
             const std::string& log_level_name);
  ~LoggerFile();
  void Write(const char* content, int len);

 private:
  void MaybeChangeLogFile();
  void UpdateSymLinkFile(const char* file);
  std::string SymLinkPath() const;

 private:
  int fd_ = -1;
  TimeStamp file_create_time_ = 0;
  std::string path_;
  std::string prog_name_;
  std::string log_level_name_;
};
}

#endif /* ----- #ifndef __LOGGER_FILE_H__  ----- */
