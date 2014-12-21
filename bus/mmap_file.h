/*
 * ==============================================================================
 *
 *       Filename:  mmap_file.h
 *        Created:  12/21/14 13:42:52
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#ifndef  __MMAP_FILE_H__
#define  __MMAP_FILE_H__

#include <string>

namespace alpha {
    class MMapFile {
        public:
            enum Flags {
                none = 0,
                truncate = 1,
                create_if_not_exists = 1 << 1,
                zero_clear = 1 << 2
            };

        public:
            MMapFile(const std::string& path, size_t size, 
                    int flags = none);
            ~MMapFile();

            bool Inited() const;
            bool newly_created() const {
                return newly_created_;
            }

            void * start() const;
            void * end() const;
            size_t size() const;

        private:
            MMapFile(const MMapFile&);
            MMapFile& operator=(const MMapFile&);

        private:
            const std::string path_;
            size_t size_;
            int fd_;
            void * start_;
            bool newly_created_;
    };
}

#endif   /* ----- #ifndef __MMAP_FILE_H__  ----- */
