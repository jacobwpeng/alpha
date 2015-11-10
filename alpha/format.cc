/*
 * ==============================================================================
 *
 *       Filename:  format.cc
 *        Created:  05/02/15 23:02:25
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#include "format.h"
#include <sstream>

namespace alpha {
std::string HexDump(alpha::Slice data) {
  char buf[10];
  std::ostringstream oss;
  const int kMaxBytesPerLine = 16;
  const int size = data.size();
  const char* bin = data.data();
  for (auto i = 0; i < size; i += kMaxBytesPerLine) {
    snprintf(buf, sizeof(buf), "%06x: ", i);
    oss << buf;
    for (auto j = 0; j < kMaxBytesPerLine; ++j) {
      if (i + j < size) {
        uint8_t c = bin[i + j];
        snprintf(buf, sizeof(buf), "%02x ", c);
        oss << buf;
      } else {
        oss << "   ";
      }
    }
    oss << " ";
    for (auto j = 0; j < kMaxBytesPerLine; ++j) {
      if (i + j < size) {
        char c = bin[i + j];
        oss << (isprint(c) ? c : '.');
      }
    }
    oss << '\n';
  }
  return oss.str();
}
}
