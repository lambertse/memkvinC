#include <gtest/gtest.h>

#include <iostream>
#include <thread>
#include <vector>

#include "ActiveMap.hpp"
#include "bitcask/Logger.hpp"

using namespace bitcask;

class ActiveMapTest : public ::testing::Test {
 protected:
  ActiveMap map;
};

TEST_F(ActiveMapTest, GetOnEmptyMapReturnsNullopt) {
  EXPECT_FALSE(map.Get("anything").has_value());
}

TEST_F(ActiveMapTest, PutThenGet) {
  map.Put("key", "value");
  auto result = map.Get("key");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "value");
}

TEST_F(ActiveMapTest, OverwriteReturnsLatestValue) {
  map.Put("key", "first");
  map.Put("key", "second");
  EXPECT_EQ(map.Get("key").value_or(""), "second");
}

TEST_F(ActiveMapTest, DeletedKeyReturnsNullopt) {
  map.Put("key", "value");
  map.Delete("key");
  EXPECT_FALSE(map.Get("key").has_value());
}

TEST_F(ActiveMapTest, DeleteNonExistentKeyIsNoop) {
  EXPECT_NO_THROW(map.Delete("nonexistent"));
}

TEST_F(ActiveMapTest, ClearRemovesAllEntries) {
  map.Put("a", "1");
  map.Put("b", "2");
  map.Put("c", "3");
  map.Clear();
  EXPECT_FALSE(map.Get("a").has_value());
  EXPECT_FALSE(map.Get("b").has_value());
  EXPECT_FALSE(map.Get("c").has_value());
}

TEST_F(ActiveMapTest, MultipleKeysStoredIndependently) {
  map.Put("x", "val_x");
  map.Put("y", "val_y");
  map.Put("z", "val_z");
  EXPECT_EQ(map.Get("x").value_or(""), "val_x");
  EXPECT_EQ(map.Get("y").value_or(""), "val_y");
  EXPECT_EQ(map.Get("z").value_or(""), "val_z");
}

TEST_F(ActiveMapTest, EmptyValueStoredAndRetrieved) {
  map.Put("key", "");
  auto result = map.Get("key");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "");
}

TEST_F(ActiveMapTest, ConcurrentPutsAllKeysReadable) {
  const int n = 100;
  std::vector<std::thread> threads;
  threads.reserve(n);
  for (int i = 0; i < n; i++) {
    threads.emplace_back(
        [&, i]() { map.Put(std::to_string(i), "v" + std::to_string(i)); });
  }
  for (auto& t : threads) t.join();

  for (int i = 0; i < n; i++)
    EXPECT_EQ(map.Get(std::to_string(i)).value_or(""), "v" + std::to_string(i))
        << "Missing key " << i;
}

int main(int argc, char** argv) {
  logger::init(logger::LOG_LEVEL_SILENCE);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
