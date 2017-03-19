/*
 * ==============================================================================
 *
 *       Filename:  main.cc
 *        Created:  04/12/15 10:19:00
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#include <cstdio>
#include <alpha/Slice.h>

void PrintSlice(alpha::Slice s) { std::printf("%s\n", s.data()); }

int main() {
  std::string s = "HELLO";
  const char* s2 = "WORLD";

  PrintSlice(s);
  PrintSlice(s2);
}
