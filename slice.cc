/*
 * ==============================================================================
 *
 *       Filename:  slice.cc
 *        Created:  12/24/14 11:09:03
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#include "slice.h"
#include <cstring>
#include <cassert>
#include <vector>

namespace alpha {

    Slice::Slice()
        : buf_ (NULL), len_ (0u) {
    }

    Slice::Slice (const char* buf, size_t len)
        : buf_ (buf), len_ (len) {
    }

    Slice::Slice (const char* s)
        : buf_ (s), len_ (::strlen (s)) {
    }

    Slice::Slice (const std::string& s)
        : buf_ (s.c_str()), len_ (s.length()) {
    }

    Slice::Slice (const Slice& slice) {
        buf_ = slice.buf_;
        len_ = slice.len_;
    }

    Slice Slice::subslice (size_t pos, size_t len) {
        if (pos == len_) {
            return Slice();
        }
        assert (pos < len_);
        if (len > (len_ - pos)) {
            return Slice (buf_ + pos, len_ - pos);
        } else {
            return Slice (buf_ + pos, len);
        }
    }

    size_t Slice::find (const Slice& s) const {
        size_t pattern_len = s.size();
        const char* p = s.data();
        if (len_ < pattern_len) {
            return npos;
        }
        /* KMP next array */
        std::vector<size_t> next (pattern_len, 0);
        if (pattern_len > 2) {
            for (size_t pos = 2; pos != pattern_len; ++pos) {
                int base = next[pos - 1];
                if (base == 0) {
                    next[pos] = p[pos] == p[0] ? 1 : 0;
                } else {
                    /* base != 0 */
                    do {
                        if (p[pos] == p[base]) {
                            break;
                        } else {
                            base = next[base - 1];
                        }
                    } while (base != 0);
                    if (base != 0) {
                        /* got match */
                        next[pos] = base + 1;
                    }
                }
            }
        }
        /* m : index in buf_, i : index in p */
        size_t m = 0, i = 0;
        while (m + i < len_) {
            /* char in source */
            char sc = buf_[m + i];
            /* char in pattern */
            char pc = p[i];
            if (sc == pc) {
                ++i;
                if (i == pattern_len) {
                    break;
                }
            } else if (i == 0) {
                ++m;
            } else {
                m += i - next[i - 1];
                i = next[i - 1];
            }
        }
        if (m + i < len_ or (m + i == len_ and i == pattern_len)) {
            /* got match */
            return m;
        } else {
            return npos;
        }
    }

    std::string Slice::ToString() const {
        return std::string (buf_, len_);
    }

    bool operator == (const Slice& lhs, const Slice& rhs) {
        return lhs.size() == rhs.size()
               and memcmp (lhs.data(), rhs.data(), lhs.size()) == 0;
    }

}

