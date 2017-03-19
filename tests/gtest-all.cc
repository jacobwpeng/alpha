/*
 * =====================================================================================
 *       Filename:  gtest-all.cc
 *        Created:  10:39:48 Apr 22, 2014
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =====================================================================================
 */

#include <gtest/gtest.h>

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
