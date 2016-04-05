/*
 * =============================================================================
 *
 *       Filename:  ThronesBattleSvrdRankVector.cc
 *        Created:  01/01/16 16:10:17
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <iterator>
#include <alpha/logger.h>
#include "ThronesBattleSvrdRankVector.h"

namespace ThronesBattle {

bool operator==(const RankVectorUnit& lhs, const RankVectorUnit& rhs) {
  return lhs.uin == rhs.uin;
}

bool operator>(const RankVectorUnit& lhs, const RankVectorUnit& rhs) {
  return lhs.val > rhs.val;
}

RankVector::RankVector(size_t max) : max_(max) {}

bool RankVector::CreateFrom(char* data, size_t size) {
  auto v = VectorType::Create(data, size);
  if (!v) {
    LOG_ERROR << "Create RankVector failed, size: " << size;
    return false;
  }
  if (v->max_size() < max_) {
    LOG_ERROR << "RankVector size is too small, expect: " << max_
              << ", actual: " << v->max_size();
    return false;
  }
  v_ = std::move(v);
  return true;
}

bool RankVector::RestoreFrom(char* data, size_t size) {
  auto v = VectorType::Restore(data, size);
  if (!v) {
    LOG_ERROR << "Restore RankVector failed, size: " << size;
    return false;
  }
  if (v->max_size() < max_) {
    LOG_ERROR << "RankVector size is too small, expect: " << max_
              << ", actual: " << v->max_size();
    return false;
  }
  v_ = std::move(v);
  return true;
}

size_t RankVector::Size() const {
  CHECK(v_);
  return v_->size();
}

void RankVector::Clear() {
  CHECK(v_);
  v_->clear();
}

void RankVector::Report(uint32_t uin, uint32_t val) {
  auto rank = Rank(uin);
  if (rank == 0) {
    // 还不在榜
    RankVectorUnit u;
    u.uin = uin;
    u.val = val;
    if (v_->size() < max_) {
      v_->push_back(u);
    } else if (u > v_->back()) {
      v_->back() = u;
    } else {
      return;
    }
  } else {
    auto it = std::next(v_->begin(), rank - 1);
    it->val = val;
  }
  KeepOrdered();
}

unsigned RankVector::Rank(uint32_t uin) const {
  CHECK(v_);
  RankVectorUnit u;
  u.uin = uin;
  auto it = std::find(v_->begin(), v_->end(), u);
  return it == v_->end() ? 0 : std::distance(v_->begin(), it) + 1;
}

std::vector<RankVectorUnit> RankVector::GetRange(unsigned start,
                                                 unsigned num) const {
  CHECK(v_);
  std::vector<RankVectorUnit> v;
  if (v_->size() <= start) {
    return v;
  }
  auto left = v_->size() - start;
  num = std::min<unsigned>(num, left);

  auto first = std::next(v_->begin(), start);
  auto last = std::next(v_->begin(), start + num);

  std::copy(first, last, std::back_inserter(v));
  return v;
}

void RankVector::KeepOrdered() {
  std::stable_sort(v_->begin(), v_->end(), std::greater<RankVectorUnit>());
}
}
