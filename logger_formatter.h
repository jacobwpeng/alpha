/*
 * ==============================================================================
 *
 *       Filename:  logger_formatter.h
 *        Created:  12/21/14 05:41:04
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#ifndef  __LOGGER_FORMATTER_H__
#define  __LOGGER_FORMATTER_H__

#include "logger_stream.h"

namespace alpha {
    class Logger;

    class LoggerFormatter
    {
        public:
            LoggerFormatter(Logger* logger, const char* basename, 
                    const char* funcname, int lineno, int level);
            ~LoggerFormatter();
            std::ostream& stream() { return stream_; }

        private:
            int FormatHeader(const char* basename, const char* funcname, 
                    int lineno, int level);

        private:
            static const size_t kMaxLogLength = 1 << 12;
            Logger* logger_;
            int log_level_;
            alpha::LoggerStream stream_;
            size_t header_len_;
            char buf[kMaxLogLength];
    };
}

#endif   /* ----- #ifndef __LOGGER_FORMATTER_H__  ----- */
