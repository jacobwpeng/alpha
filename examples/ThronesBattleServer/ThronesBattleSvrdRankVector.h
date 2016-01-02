/*
 * =============================================================================
 *
 *       Filename:  ThronesBattleSvrdRankVector.h
 *        Created:  01/01/16 16:06:25
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#pragma once

#include <alpha/experimental/RegionBasedVector.h>

namespace ThronesBattle {
struct RankVectorUnit {
  uint32_t uin;
  uint32_t val;
};

bool operator==(const RankVectorUnit& lhs, const RankVectorUnit& rhs);
bool operator>(const RankVectorUnit& lhs, const RankVectorUnit& rhs);

class RankVector {
 public:
  RankVector(size_t max);

  bool CreateFrom(char* data, size_t size);
  bool RestoreFrom(char* data, size_t size);

  size_t Size() const;
  void Clear();
  void Report(uint32_t uin, uint32_t val);
  void ReportDelta(uint32_t uin, uint32_t delta);
  unsigned Rank(uint32_t uin) const;
  // start 从0开始
  std::vector<RankVectorUnit> GetRange(unsigned start, unsigned num) const;

 private:
  void KeepOrdered();
  size_t max_;
  using VectorType = alpha::RegionBasedVector<RankVectorUnit>;
  std::unique_ptr<VectorType> v_;
};
}
