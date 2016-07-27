/*
 * =============================================================================
 *
 *       Filename:  RingBufferTest.cc
 *        Created:  03/09/16 16:34:49
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <string>
#include <memory>
#include <gtest/gtest.h>
#include <alpha/logger.h>
#include <alpha/compiler.h>
#include <alpha/RingBuffer.h>

class RingBufferTest : public ::testing::Test {
 protected:
  virtual void SetUp() override {
    buffer_ = alpha::make_unique<alpha::RingBuffer>();
    bool ok = buffer_->CreateFrom(underlying_buffer_, kBufferSize);
    CHECK(ok);
  }

  virtual void TearDown() override {
    memset(underlying_buffer_, 0x0, kBufferSize);
    buffer_ = nullptr;
  }

  static const int kBufferSize = alpha::RingBuffer::kMaxBufferBodyLength * 5;
  char underlying_buffer_[kBufferSize];
  std::unique_ptr<alpha::RingBuffer> buffer_;
};

TEST_F(RingBufferTest, CreateFrom) {
  ASSERT_GT(alpha::RingBuffer::kMinByteSize, 0);
  alpha::RingBuffer buffer;
  bool ok = buffer.CreateFrom(nullptr, alpha::RingBuffer::kMinByteSize);
  EXPECT_FALSE(ok);

  char buf[kBufferSize];
  ASSERT_GE(sizeof(buf), (size_t)alpha::RingBuffer::kMinByteSize);
  ok = buffer.CreateFrom(buf, alpha::RingBuffer::kMinByteSize - 1);
  EXPECT_FALSE(ok);

  ok = buffer.CreateFrom(buf, alpha::RingBuffer::kMinByteSize);
  EXPECT_TRUE(ok);
}

TEST_F(RingBufferTest, RestoreFrom) {
  std::string msg = "Long live the queen!";
  bool ok = buffer_->Push(msg.data(), msg.size());
  ASSERT_TRUE(ok);

  alpha::RingBuffer buffer;
  ok = buffer.RestoreFrom(underlying_buffer_, kBufferSize);
  EXPECT_TRUE(ok);
  int len;
  void* data = buffer.Pop(&len);
  EXPECT_NE(data, nullptr);
  EXPECT_EQ((size_t)len, msg.size());
  EXPECT_EQ(memcmp(msg.data(), data, len), 0);
}

TEST_F(RingBufferTest, Push) {
  bool ok = buffer_->Push(nullptr, 1024);
  EXPECT_FALSE(ok);
  ok = buffer_->Push(underlying_buffer_, 0);
  EXPECT_FALSE(ok);
  std::vector<char> buf(alpha::RingBuffer::kMaxBufferBodyLength + 1, 0x3f);
  ok = buffer_->Push(buf.data(), buf.size());
  EXPECT_FALSE(ok);
  ok = buffer_->Push(buf.data(), alpha::RingBuffer::kMaxBufferBodyLength);
  EXPECT_TRUE(ok);
  std::string msg = "Long live the queen!";
  ok = buffer_->Push(msg.data(), msg.size());
  EXPECT_TRUE(ok);
}

TEST_F(RingBufferTest, Pop) {
  int len = -1;
  void* data = buffer_->Pop(&len);
  EXPECT_EQ(data, nullptr);
  EXPECT_EQ(len, 0);

  std::string msg1 = "Long live the queen!";
  std::string msg2 = "The quick brown fox jumps over the lazy dog";

  bool ok = buffer_->Push(msg1.data(), msg1.size());
  ASSERT_TRUE(ok);
  ok = buffer_->Push(msg2.data(), msg2.size());
  ASSERT_TRUE(ok);

  data = buffer_->Pop(&len);
  EXPECT_EQ((size_t)len, msg1.size());
  EXPECT_EQ(memcmp(data, msg1.data(), len), 0);

  void* data2 = buffer_->Pop(&len);
  EXPECT_EQ((size_t)len, msg2.size());
  EXPECT_EQ(memcmp(data2, msg2.data(), len), 0);

  // Peek/Pop的结果有效周期只能到下一次Peek/Pop操作/RingBuffer析构
  EXPECT_NE(memcmp(data, msg1.data(), msg1.size()), 0);

  data = buffer_->Pop(&len);
  EXPECT_EQ(data, nullptr);
  EXPECT_EQ(len, 0);
}

TEST_F(RingBufferTest, Peek) {
  int len = -1;
  void* data = buffer_->Pop(&len);
  EXPECT_EQ(data, nullptr);
  EXPECT_EQ(len, 0);

  std::string msg1 = "Long live the queen!";
  std::string msg2 = "The quick brown fox jumps over the lazy dog";

  bool ok = buffer_->Push(msg1.data(), msg1.size());
  ASSERT_TRUE(ok);
  ok = buffer_->Push(msg2.data(), msg2.size());
  ASSERT_TRUE(ok);

  data = buffer_->Peek(&len);
  EXPECT_EQ((size_t)len, msg1.size());
  EXPECT_EQ(memcmp(data, msg1.data(), len), 0);

  void* data2 = buffer_->Pop(&len);
  EXPECT_EQ((size_t)len, msg1.size());
  EXPECT_EQ(memcmp(data2, msg1.data(), len), 0);

  data = buffer_->Pop(&len);
  EXPECT_NE(data, nullptr);
  EXPECT_EQ(len, msg2.size());
}

TEST_F(RingBufferTest, empty) {
  EXPECT_TRUE(buffer_->empty());
  std::string msg = "Long live the queen!";
  buffer_->Push(msg.data(), msg.size());
  EXPECT_FALSE(buffer_->empty());
  int len;
  buffer_->Pop(&len);
  EXPECT_TRUE(buffer_->empty());
}

TEST_F(RingBufferTest, TestExtraSpace) {
  // 通过ExtraSpace来区分队列满和队列空两种情况
  EXPECT_TRUE(buffer_->empty());
  std::vector<char> buf(alpha::RingBuffer::kMaxBufferBodyLength, 0x3f);
  while (buffer_->SpaceLeft()) {
    int sz = std::min<int>(buf.size(), buffer_->SpaceLeft());
    buffer_->Push(buf.data(), sz);
  }

  EXPECT_EQ(buffer_->SpaceLeft(), 0);
  EXPECT_FALSE(buffer_->empty());
}

TEST_F(RingBufferTest, TestFulfil) {
  // 某版本Push时计算SpaceLeft没有把int32_t的长度前缀考虑在内
  // 导致后续Push时覆盖了之前Push的内容
  char c = 0x3f;
  int num = 0;
  while (buffer_->Push(&c, 1)) {
    ++num;
  }

  int len;
  while (void* data = buffer_->Pop(&len)) {
    EXPECT_EQ(*reinterpret_cast<char*>(data), c);
    --num;
  }

  EXPECT_EQ(num, 0);
}
