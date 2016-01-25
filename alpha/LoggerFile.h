/*
 * ==============================================================================
 *
 *       Filename:  LoggerFile.h
 *        Created:  03/27/15 21:27:20
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#pragma once

#include <string>
#include <alpha/slice.h>
#include <alpha/TimeUtil.h>
#include <alpha/File.h>

namespace alpha {
class Slice;
class LoggerFile {
 public:
  LoggerFile(const std::string& path, const std::string& prog_name,
             const std::string& log_level_name);
  void Write(const char* content, int len);

 private:
  void MaybeChangeLogFile();
  void UpdateSymLinkFile(const char* file);
  std::string SymLinkPath() const;

 private:
  File file_;
  TimeStamp file_create_time_{0};
  std::string path_;
  std::string prog_name_;
  std::string log_level_name_;
};
}
