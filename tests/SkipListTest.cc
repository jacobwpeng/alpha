/*
 * ==============================================================================
 *
 *       Filename:  SkipListTest.cc
 *        Created:  06/05/15 22:06:13
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * ==============================================================================
 */

#include <map>
#include <vector>
#include <gtest/gtest.h>
#include <alpha/SkipList.h>
#include <alpha/Random.h>

class SkipListTest : public ::testing::Test {
 protected:
  using DefaultSkipListType = alpha::SkipList<int, int>;

  virtual void SetUp() {
    buffer_.resize(kBufferSize);
    list_ = DefaultSkipListType::Create(buffer_.data(), buffer_.size());
  }

  virtual void TearDown() { buffer_.clear(); }

  static const size_t kBufferSize = (1 << 23);
  std::vector<char> buffer_;
  DefaultSkipListType::UniquePtr list_;
};

TEST_F(SkipListTest, Create) {
  ASSERT_NE(list_, nullptr);
  EXPECT_EQ(list_->size(), 0u);
  EXPECT_NE(list_->max_size(), 0u);
  EXPECT_TRUE(list_->empty());
  EXPECT_EQ(list_->begin(), list_->end());
}

TEST_F(SkipListTest, insert) {
  ASSERT_NE(list_, nullptr);
  ASSERT_TRUE(list_->empty());
  const auto max_size = list_->max_size();

  int key = -1;
  int val = 2;
  auto res = list_->insert(std::make_pair(key, val));
  EXPECT_EQ(list_->size(), 1u);
  EXPECT_EQ(res.first, list_->begin());
  EXPECT_TRUE(res.second);
  EXPECT_EQ(res.first->first, key);
  EXPECT_EQ(res.first->second, val);
  EXPECT_FALSE(list_->empty());
  EXPECT_EQ(max_size, list_->max_size());

  auto it = list_->find(key);
  EXPECT_EQ(it, res.first);

  int newval = 3;
  auto newres = list_->insert(std::make_pair(key, newval));
  EXPECT_EQ(list_->size(), 1u);
  EXPECT_EQ(newres.first, res.first);
  EXPECT_FALSE(newres.second);
  EXPECT_EQ(newres.first->second, val);
  EXPECT_NE(newres.first->second, newval);

  int newkey = 0;
  newres = list_->insert(std::make_pair(newkey, newval));
  EXPECT_EQ(list_->size(), 2u);
  EXPECT_TRUE(newres.second);
  EXPECT_EQ(newres.first->second, newval);
}

TEST_F(SkipListTest, bracketoperator) {
  ASSERT_NE(list_, nullptr);
  ASSERT_TRUE(list_->empty());

  int key = 1;
  auto& val = (*list_)[key];
  val = 3;

  EXPECT_EQ(list_->size(), 1u);
  EXPECT_FALSE(list_->empty());
  auto it = list_->begin();
  EXPECT_NE(it, list_->end());
  EXPECT_EQ(it->first, key);
  EXPECT_EQ(it->second, val);

  val = 4;
  EXPECT_EQ(it->second, val);

  (*list_)[key] = 5;
  EXPECT_EQ(it->second, 5);

  int newkey = 1024;
  (*list_)[newkey] = 2;
  EXPECT_EQ(list_->size(), 2u);
  EXPECT_FALSE(list_->empty());
}

TEST_F(SkipListTest, find) {
  ASSERT_NE(list_, nullptr);
  ASSERT_TRUE(list_->empty());
  std::map<int, int> m = {{std::numeric_limits<int>::min(), -1},
                          {-1024, 0xffff},
                          {-8, 0xaaaa},
                          {0, 0xbbbb},
                          {128, 0xcccc},
                          {std::numeric_limits<int>::max(), 0}};

  for (const auto& p : m) {
    auto res = list_->insert(p);
    ASSERT_TRUE(res.second);
  }

  ASSERT_EQ(m.size(), list_->size());
  auto key = std::numeric_limits<int>::min();
  auto it = list_->find(key);
  EXPECT_NE(it, list_->end());
  EXPECT_EQ(it->first, key);
  EXPECT_EQ(it->second, m[key]);

  key = 0;
  it = list_->find(key);
  EXPECT_NE(it, list_->end());
  EXPECT_EQ(it->first, key);
  EXPECT_NE(it->second, key);

  key = 1024;  // Not exist key
  it = list_->find(key);
  EXPECT_EQ(it, list_->end());
}

TEST_F(SkipListTest, erase) {
  ASSERT_NE(list_, nullptr);
  ASSERT_TRUE(list_->empty());

  (*list_)[-1] = 0xabcd;
  (*list_)[0] = 0xdcba;
  (*list_)[1] = 0x1234;
  (*list_)[2] = 0x4321;
  (*list_)[3] = 0x1234567;
  (*list_)[4] = 0;
  (*list_)[5] = 7;

  const size_t keys = 7;
  ASSERT_EQ(list_->size(), keys);

  // erase by key
  auto num = list_->erase(-1);
  EXPECT_EQ(num, 1u);
  EXPECT_EQ(list_->size(), keys - 1);

  // erase by iterator
  auto it = list_->find(1);
  ASSERT_NE(it, list_->end());
  list_->erase(it);
  EXPECT_EQ(list_->size(), keys - 2);

  // erase non-exist key
  int non_exist_key = 0x1234;
  ASSERT_EQ(list_->find(non_exist_key), list_->end());
  num = list_->erase(non_exist_key);
  EXPECT_EQ(num, 0u);
  EXPECT_EQ(list_->size(), keys - 2);

  // erase elment in the middle
  num = list_->erase(4);
  EXPECT_EQ(num, 1u);
  EXPECT_EQ(list_->size(), keys - 3);

  // erase range
  auto first = list_->begin();
  auto last = list_->find(5);
  EXPECT_NE(first, list_->end());
  EXPECT_NE(last, list_->end());
  EXPECT_EQ(first->first, 0);

  list_->erase(first, last);
  EXPECT_EQ(list_->size(), 1u);

  // erase begin
  list_->erase(list_->begin());
  EXPECT_EQ(list_->size(), 0u);
  EXPECT_TRUE(list_->empty());
}

TEST_F(SkipListTest, clear) {
  ASSERT_NE(list_, nullptr);
  ASSERT_TRUE(list_->empty());

  (*list_)[3] = 5;
  EXPECT_FALSE(list_->empty());

  list_->clear();
  EXPECT_TRUE(list_->empty());
}

TEST_F(SkipListTest, iterator) {
  ASSERT_NE(list_, nullptr);
  ASSERT_TRUE(list_->empty());

  int key = 5;
  int val = 0;

  EXPECT_EQ(list_->begin(), list_->end());
  (*list_)[key] = val;
  EXPECT_NE(list_->begin(), list_->end());

  // test value
  auto it = list_->find(key);
  EXPECT_NE(it, list_->end());
  EXPECT_EQ(it->first, key);
  EXPECT_EQ(it->second, val);

  // test bidirection
  auto first = list_->begin();
  ++first;
  EXPECT_EQ(first, list_->end());

  auto last = list_->end();
  --last;
  EXPECT_EQ(list_->begin(), last);

  list_->clear();
  ASSERT_TRUE(list_->empty());

  // test traversal
  std::map<int, int> m = {{std::numeric_limits<int>::min(), -1},
                          {-1024, 0xffff},
                          {-8, 0xaaaa},
                          {0, 0xbbbb},
                          {128, 0xcccc},
                          {std::numeric_limits<int>::max(), 0}};
  for (const auto& p : m) {
    list_->insert(p);
  }
  ASSERT_EQ(m.size(), list_->size());
  for (auto it = list_->begin(); it != list_->end(); ++it) {
    auto iter = m.find(it->first);
    EXPECT_NE(iter, m.end());
    EXPECT_EQ(it->first, iter->first);
    EXPECT_EQ(it->second, iter->second);
  }

  it = list_->end();
  do {
    --it;
    auto iter = m.find(it->first);
    EXPECT_NE(iter, m.end());
    EXPECT_EQ(it->first, iter->first);
    EXPECT_EQ(it->second, iter->second);
  } while (it != list_->begin());

  // test iterator invalidation
  it = list_->find(0);
  ASSERT_NE(it, list_->end());
  ASSERT_EQ(it->first, 0);

  auto num = list_->erase(-8);
  ASSERT_EQ(num, 1u);
  ASSERT_NE(it, list_->end());
  ASSERT_EQ(it->first, 0);

  num = list_->erase(128);
  ASSERT_EQ(num, 1u);
  ASSERT_NE(it, list_->end());
  ASSERT_EQ(it->first, 0);
}

TEST_F(SkipListTest, Restore) {
  ASSERT_NE(list_, nullptr);
  ASSERT_TRUE(list_->empty());

  const size_t expected_elements = list_->max_size();
  for (auto i = 0u; i < expected_elements; ++i) {
    (*list_)[i] = alpha::Random::Rand32();
  }

  ASSERT_EQ(list_->size(), expected_elements);
  ASSERT_EQ(list_->size(), list_->max_size());
  ASSERT_FALSE(list_->empty());

  decltype(buffer_) newbuffer(buffer_);

  auto newlist =
      DefaultSkipListType::Restore(newbuffer.data(), newbuffer.size());
  ASSERT_NE(newlist, nullptr);
  EXPECT_EQ(newlist->size(), list_->size());
  EXPECT_EQ(newlist->max_size(), list_->max_size());
  EXPECT_NE(newlist->begin(), list_->begin());
  EXPECT_NE(newlist->end(), list_->end());

  EXPECT_TRUE(std::lexicographical_compare(
      list_->begin(),
      list_->end(),
      newlist->begin(),
      newlist->end(),
      [](const DefaultSkipListType::value_type& lhs,
         const DefaultSkipListType::value_type& rhs) {
        return lhs.first == rhs.first && lhs.second == rhs.second;
      }));

  // test separation
  auto it = newlist->begin();
  auto oldval = it->second;
  auto newval = (oldval == 0 ? 1 : 0);
  it->second = newval;
  EXPECT_EQ(newlist->begin()->second, newval);
  EXPECT_EQ(list_->begin()->second, oldval);

  newlist->clear();
  ASSERT_TRUE(newlist->empty());
  ASSERT_EQ(list_->size(), expected_elements);
}
