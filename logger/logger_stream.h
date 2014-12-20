/*
 * ==============================================================================
 *
 *       Filename:  logger_stream.h
 *        Created:  12/21/14 05:42:38
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#ifndef  __LOGGER_STREAM_H__
#define  __LOGGER_STREAM_H__

#include <ostream>
#include "logger_streambuf.h"

namespace alpha {
    class LoggerStream : public std::ostream {
        public:
            LoggerStream();

            LoggerStreambuf* streambuf();

        private:
            LoggerStreambuf streambuf_;
    };
}

#endif   /* ----- #ifndef __LOGGER_STREAM_H__  ----- */
