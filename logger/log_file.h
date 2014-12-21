/*
 * ==============================================================================
 *
 *       Filename:  log_file.h
 *        Created:  12/21/14 06:55:04
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#ifndef  __LOG_FILE_H__
#define  __LOG_FILE_H__

#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)

namespace alpha {
    class LogFile {
        public:
            LogFile(const char* file);
            ~LogFile();

            void Append(const char* content, int len);

        private:
            bool MaybeCreateFile();

            int fd_;
            const char* file_;
    };
}

#endif   /* ----- #ifndef __LOG_FILE_H__  ----- */
