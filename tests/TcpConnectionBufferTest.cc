/*
 * =============================================================================
 *
 *       Filename:  TcpConnectionBufferTest.cc
 *        Created:  02/04/16 23:38:07
 *         Author:  Peng Wang
 *          Email:  pw2191195@gmail.com
 *    Description:
 *
 * =============================================================================
 */

#include <cstring>
#include <limits>
#include <algorithm>
#include <gtest/gtest.h>
#include <alpha/tcp_connection_buffer.h>

TEST(TcpConnectionBufferTest, Append) {
  alpha::TcpConnectionBuffer buffer;
  auto length = buffer.GetContiguousSpace();
  if (length != 0) {
    char c = 0x3f;
    EXPECT_TRUE(buffer.Append(&c, 1));
    std::vector<char> buf(length - 1);
    EXPECT_TRUE(buffer.Append(buf.data(), buf.size()));
    buf.resize(alpha::TcpConnectionBuffer::kMaxBufferSize);
    EXPECT_FALSE(buffer.Append(buf.data(), buf.size()));
    EXPECT_EQ(buffer.SpaceBeforeFull(),
              alpha::TcpConnectionBuffer::kMaxBufferSize - length);
    EXPECT_TRUE(buffer.Append(
        buf.data(), alpha::TcpConnectionBuffer::kMaxBufferSize - length));
    EXPECT_FALSE(buffer.Append(&c, 1));
  }
}

TEST(TcpConnectionBufferTest, WriteInternalBuffer) {
  alpha::TcpConnectionBuffer buffer;
  auto length = buffer.GetContiguousSpace();
  auto p = buffer.WriteBegin();
  memset(p, 0x3f, length);
  buffer.AddBytes(length);
  EXPECT_EQ(buffer.GetContiguousSpace(), 0u);
  EXPECT_EQ(buffer.SpaceBeforeFull(),
            alpha::TcpConnectionBuffer::kMaxBufferSize -
                alpha::TcpConnectionBuffer::kDefaultBufferSize);
  EXPECT_FALSE(buffer.AddBytes(1));
}

TEST(TcpConnectionBufferTest, GetContiguousSpace) {
  alpha::TcpConnectionBuffer buffer;
  auto length = buffer.GetContiguousSpace();
  EXPECT_EQ(length, alpha::TcpConnectionBuffer::kDefaultBufferSize);
  const std::string s = "The final answer is 42!";
  ASSERT_LE(s.size(), alpha::TcpConnectionBuffer::kDefaultBufferSize);
  buffer.Append(s);
  length = buffer.GetContiguousSpace();
  EXPECT_EQ(length, alpha::TcpConnectionBuffer::kDefaultBufferSize - s.size());
  buffer.ConsumeBytes(s.size());
  length = buffer.GetContiguousSpace();
  EXPECT_EQ(length, alpha::TcpConnectionBuffer::kDefaultBufferSize);

  // force reallocate memory
  ASSERT_LT(alpha::TcpConnectionBuffer::kDefaultBufferSize,
            alpha::TcpConnectionBuffer::kMaxBufferSize);
  std::vector<char> tmp(alpha::TcpConnectionBuffer::kDefaultBufferSize + 1);
  buffer.Append(tmp.data(), tmp.size());
  length = buffer.GetContiguousSpace();
  ASSERT_LE(buffer.BytesToRead(), buffer.capacity());
  EXPECT_EQ(length, buffer.capacity() - buffer.BytesToRead());
}

TEST(TcpConnectionBufferTest, ReadNullTerminatedString) {
  alpha::TcpConnectionBuffer buffer;
  size_t length;
  auto p = buffer.Read(&length);
  EXPECT_EQ(p, nullptr);

  const std::string s = "The answer is 42";
  auto ok = buffer.Append(s);
  ASSERT_TRUE(ok);
  p = buffer.Read(&length);
  ASSERT_NE(p, nullptr);
  ASSERT_EQ(length, s.size());
  EXPECT_EQ(memcmp(p, s.data(), length), 0);
}

TEST(TcpConnectionBufferTest, ReadRawBytes) {
  alpha::TcpConnectionBuffer buffer;
  std::vector<char> bytes(alpha::TcpConnectionBuffer::kDefaultBufferSize);
  memset(bytes.data(), 0x5e, bytes.size());
  auto ok = buffer.Append(bytes.data(), bytes.size());
  ASSERT_TRUE(ok);

  size_t length;
  auto p = buffer.Read(&length);
  ASSERT_NE(p, nullptr);
  ASSERT_EQ(length, bytes.size());
  EXPECT_EQ(memcmp(p, bytes.data(), length), 0);
}

TEST(TcpConnectionBufferTest, ConsumeBytes) {
  alpha::TcpConnectionBuffer buffer;
  const std::string s = "Balabala";
  buffer.Append(s);
  size_t length;
  auto p = buffer.Read(&length);

  EXPECT_NE(p, nullptr);
  EXPECT_EQ(length, s.size());

  buffer.ConsumeBytes(1);
  p = buffer.Read(&length);
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(length, s.size() - 1);
  EXPECT_EQ(memcmp(s.data() + 1, p, length), 0);

  buffer.ConsumeBytes(length);
  p = buffer.Read(&length);
  ASSERT_EQ(p, nullptr);
}

TEST(TcpConnectionBufferTest, ReadAndClear) {
  alpha::TcpConnectionBuffer buffer;
  const std::string s = "The answer is 42";
  auto ok = buffer.Append(s);
  ASSERT_TRUE(ok);

  size_t length;
  buffer.Read(&length);
  ASSERT_EQ(length, s.size());

  std::vector<char> saved(length);
  auto n = buffer.ReadAndClear(saved.data(), length);
  ASSERT_EQ(n, length);
  EXPECT_EQ(memcmp(saved.data(), s.data(), length), 0);

  n = buffer.ReadAndClear(saved.data(), length);
  EXPECT_EQ(n, 0u);

  saved.clear();
  saved.resize(alpha::TcpConnectionBuffer::kDefaultBufferSize, 0x3f);
  ok = buffer.Append(saved.data(), saved.size());
  ASSERT_TRUE(ok);

  std::vector<char> tmp(saved.size() + 1, 0x6f);
  // Read more than we have
  n = buffer.ReadAndClear(tmp.data(), saved.size() + 1);
  EXPECT_EQ(n, saved.size());
  EXPECT_EQ(tmp[saved.size()], 0x6f);
  EXPECT_EQ(memcmp(tmp.data(), saved.data(), saved.size()), 0);

  tmp.clear();
  saved.clear();
  saved.resize(alpha::TcpConnectionBuffer::kMaxBufferSize, 0x3f);

  ok = buffer.Append(saved.data(), saved.size());
  EXPECT_TRUE(ok);
}

TEST(TcpConnectionBufferTest, ReadAndWrite) {
  alpha::TcpConnectionBuffer buffer;
  static const size_t kLength = 100;
  std::vector<char> bytes(kLength, 0x5f);
  std::vector<char> out(kLength);
  auto ok = buffer.Append(bytes.data(), 10);
  ASSERT_TRUE(ok);

  size_t length;
  auto p = buffer.Read(&length);
  EXPECT_NE(p, nullptr);
  EXPECT_EQ(length, 10u);

  auto n = buffer.ReadAndClear(out.data(), 3);
  p = buffer.Read(&length);

  EXPECT_NE(p, nullptr);
  EXPECT_EQ(n, 3u);
  EXPECT_EQ(length, 7u);

  memset(bytes.data(), 0x6f, bytes.size());
  ok = buffer.Append(bytes.data(), 20);
  ASSERT_TRUE(ok);
  p = buffer.Read(&length);

  EXPECT_NE(p, nullptr);
  EXPECT_EQ(length, 27u);

  n = buffer.ReadAndClear(out.data(), std::numeric_limits<size_t>::max());
  EXPECT_EQ(n, 27u);

  auto first = std::begin(out);
  auto last = std::next(first, 7);
  auto it = std::find_if(first, last, [](char c) { return c != 0x5f; });
  EXPECT_TRUE(it == last);
  first = last;
  last = std::next(first, 20);
  it = std::find_if(first, last, [](char c) { return c != 0x6f; });
  EXPECT_TRUE(it == last);
}
