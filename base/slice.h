/*
 * ==============================================================================
 *
 *       Filename:  slice.h
 *        Created:  12/24/14 11:07:57
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:  
 *
 * ==============================================================================
 */

#ifndef  __SLICE_H__
#define  __SLICE_H__

#include <cstddef>
#include <string>

namespace alpha {

    class Slice {
        public:
            static const size_t npos = (size_t)(-1);

        public:
            Slice();
            Slice(const char * s, size_t len);
            Slice(const char * s);
            Slice(const std::string& str);
            Slice(const Slice& slice);

            Slice subslice(size_t pos = 0, size_t len = npos);

            bool empty() const { return buf_ == NULL; }
            const char * data() const { return buf_; }
            size_t size() const { return len_; }
            /* KMP style find using next array */
            size_t find(const Slice& s) const;

            std::string ToString() const;

        private:
            /* KMP style find using DFA */
            size_t InternalFind( const char * p ) const;

        private:
            const char * buf_;
            size_t len_;
    };

    bool operator == (const Slice & lhs, const Slice & rhs);
}

#endif   /* ----- #ifndef __SLICE_H__  ----- */
