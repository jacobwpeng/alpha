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

#ifndef  __LOGGER_FILE_H__
#define  __LOGGER_FILE_H__

#include <string>
#include "slice.h"
#include "time_util.h"

namespace alpha {
    enum class LogLevel : short;
    class Slice;
    class LoggerFile {
        public:
            LoggerFile(const Slice& path, const Slice& prog_name);
            ~LoggerFile();

            void Write(LogLevel level, const char* content, int len);

        private:
            void MaybeChangeLogFile();

        private:
            int fd_;
            TimeStamp file_create_time_;
            std::string path_;
            std::string prog_name_;
    };
}

#endif   /* ----- #ifndef __LOGGER_FILE_H__  ----- */
