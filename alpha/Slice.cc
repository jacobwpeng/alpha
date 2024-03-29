/*
 * ==============================================================================
 *
 *       Filename:  Slice.cc
 *        Created:  12/24/14 11:09:03
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#include <alpha/Slice.h>
#include <cstring>
#include <cassert>
#include <vector>
#include <ostream>

namespace alpha {

Slice::Slice() : buf_(NULL), len_(0u) {}

Slice::Slice(const char* buf, size_t len) : buf_(buf), len_(len) {}

Slice::Slice(const char* s) : buf_(s), len_(::strlen(s)) {}

Slice::Slice(const std::string& s) : buf_(s.c_str()), len_(s.length()) {}

Slice::Slice(const Slice& slice) {
  buf_ = slice.buf_;
  len_ = slice.len_;
}

Slice& Slice::operator=(const Slice& rhs) {
  if (this != &rhs) {
    buf_ = rhs.buf_;
    len_ = rhs.len_;
  }
  return *this;
}

Slice Slice::subslice(size_t pos, size_t len) {
  if (pos == len_) {
    return Slice();
  }
  assert(pos < len_);
  return Slice(buf_ + pos, std::min(len, len_ - pos));
}

size_t Slice::find(const Slice& s) const {
  size_t pattern_len = s.size();
  const char* p = s.data();
  if (len_ < pattern_len) {
    return npos;
  }
  /* KMP next array */
  std::vector<size_t> next(pattern_len, 0);
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

void Slice::Clear() {
  buf_ = nullptr;
  len_ = 0;
}

bool Slice::StartsWith(alpha::Slice prefix) const {
  return len_ >= prefix.len_ && memcmp(buf_, prefix.buf_, prefix.len_) == 0;
}

bool Slice::EndsWith(Slice suffix) const {
  return len_ >= suffix.len_ &&
         memcmp(buf_ + len_ - suffix.len_, suffix.buf_, suffix.len_) == 0;
}

bool Slice::RemovePrefix(Slice prefix) {
  return StartsWith(prefix) && (Advance(prefix.size()), true);
}

bool Slice::RemoveSuffix(Slice suffix) {
  return EndsWith(suffix) && (Subtract(suffix.size()), true);
}

void Slice::Advance(size_t n) {
  assert(size() >= n);
  buf_ += n;
  len_ -= n;
}

void Slice::Subtract(size_t n) {
  assert(size() >= n);
  len_ -= n;
}

std::string Slice::ToString() const { return std::string(buf_, len_); }

bool operator<(const Slice& lhs, const Slice& rhs) {
  return std::lexicographical_compare(
      lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

bool operator==(const Slice& lhs, const Slice& rhs) {
  return lhs.size() == rhs.size() and
         memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
}

std::ostream& operator<<(std::ostream& os, const Slice& s) {
  os.write(s.data(), s.size());
  return os;
}
}  // namespace alpha
