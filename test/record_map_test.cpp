#include "RecordMap.hpp"
#include "bitcask/Logger.hpp"
#include <gtest/gtest.h>
#include <iostream>
#include <thread>
#include <vector>

using namespace bitcask;

class RecordMapTest : public ::testing::Test {
protected:
  RecordMap rmap;
};

TEST_F(RecordMapTest, GetOnEmptyMapReturnsNullopt) {
  EXPECT_FALSE(rmap.Get("anything").has_value());
}

TEST_F(RecordMapTest, PutThenGet) {
  Hint h{1, 42, 10};
  rmap.Put("key", h);
  auto result = rmap.Get("key");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->fd, 1u);
  EXPECT_EQ(result->offset, 42u);
  EXPECT_EQ(result->size, 10u);
}

TEST_F(RecordMapTest, OverwriteReturnsLatestHint) {
  rmap.Put("key", Hint{0, 10, 5});
  rmap.Put("key", Hint{2, 99, 7});
  auto result = rmap.Get("key");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->fd, 2u);
  EXPECT_EQ(result->offset, 99u);
  EXPECT_EQ(result->size, 7u);
}

TEST_F(RecordMapTest, DeletedKeyReturnsNullopt) {
  rmap.Put("key", Hint{1, 0, 4});
  rmap.Delete("key");
  EXPECT_FALSE(rmap.Get("key").has_value());
}

TEST_F(RecordMapTest, DeleteNonExistentKeyIsNoop) {
  EXPECT_NO_THROW(rmap.Delete("ghost"));
}

TEST_F(RecordMapTest, ClearRemovesAllEntries) {
  rmap.Put("a", Hint{0, 0, 1});
  rmap.Put("b", Hint{0, 1, 2});
  rmap.Put("c", Hint{0, 2, 3});
  rmap.Clear();
  EXPECT_FALSE(rmap.Get("a").has_value());
  EXPECT_FALSE(rmap.Get("b").has_value());
  EXPECT_FALSE(rmap.Get("c").has_value());
}

TEST_F(RecordMapTest, MultipleKeysStoredIndependently) {
  rmap.Put("x", Hint{0, 10, 3});
  rmap.Put("y", Hint{1, 20, 4});
  rmap.Put("z", Hint{2, 30, 5});
  EXPECT_EQ(rmap.Get("x")->fd, 0u);
  EXPECT_EQ(rmap.Get("y")->fd, 1u);
  EXPECT_EQ(rmap.Get("z")->fd, 2u);
}

TEST_F(RecordMapTest, DefaultHintHasZeroFields) {
  Hint h;
  EXPECT_EQ(h.fd, 0u);
  EXPECT_EQ(h.offset, 0u);
  EXPECT_EQ(h.size, 0u);
}

TEST_F(RecordMapTest, ConcurrentPutsAllKeysReadable) {
  const int n = 100;
  std::vector<std::thread> threads;
  threads.reserve(n);
  for (int i = 0; i < n; i++) {
    threads.emplace_back([&, i]() {
      rmap.Put(std::to_string(i), Hint{(uint32_t)i, (uint32_t)(i * 10), 4u});
    });
  }
  for (auto &t : threads)
    t.join();

  for (int i = 0; i < n; i++) {
    auto h = rmap.Get(std::to_string(i));
    ASSERT_TRUE(h.has_value()) << "Missing key " << i;
    EXPECT_EQ(h->fd, (uint32_t)i);
    EXPECT_EQ(h->offset, (uint32_t)(i * 10));
  }
}

int main(int argc, char **argv) {
  logger::init(logger::LOG_LEVEL_SILENCE);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
