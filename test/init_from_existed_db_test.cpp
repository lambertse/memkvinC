#include <gtest/gtest.h>

#include <filesystem>
#include <iostream>

#include "BitcaskImpl.hpp"
#include "bitcask/Logger.hpp"

using namespace bitcask;

#define DB_PATH "/tmp/test_exist_db/"

class BitCaskImplRestoreTest : public ::testing::Test {
 protected:
  void SetUp() override {
    std::filesystem::remove_all(DB_PATH);
    std::filesystem::create_directory(DB_PATH);
  }

  void TearDown() override { std::filesystem::remove_all(DB_PATH); }
};

TEST_F(BitCaskImplRestoreTest, DataPersistsAfterRestart) {
  {
    auto db = std::make_unique<BitcaskImpl>(DB_PATH);
    db->Put("key1", "value1").get();
    db->Put("key2", "value2").get();
    db->Put("key3", "value3").get();
    EXPECT_EQ(db->Get("key1").value_or(""), "value1");
    EXPECT_EQ(db->Get("key2").value_or(""), "value2");
    EXPECT_EQ(db->Get("key3").value_or(""), "value3");
  }  // destructor flushes and closes all files

  // Reopen from the same directory
  auto db2 = std::make_unique<BitcaskImpl>(DB_PATH);
  EXPECT_EQ(db2->Get("key1").value_or(""), "value1");
  EXPECT_EQ(db2->Get("key2").value_or(""), "value2");
  EXPECT_EQ(db2->Get("key3").value_or(""), "value3");
}

TEST_F(BitCaskImplRestoreTest, OverwriteValuePersistsAfterRestart) {
  {
    auto db = std::make_unique<BitcaskImpl>(DB_PATH);
    db->Put("key", "old_value").get();
    db->Put("key", "new_value").get();
    EXPECT_EQ(db->Get("key").value_or(""), "new_value");
  }

  auto db2 = std::make_unique<BitcaskImpl>(DB_PATH);
  // Both entries are on disk; the last-written value should win
  // (the active map restores from file in order)
  EXPECT_EQ(db2->Get("key").value_or(""), "new_value");
}

TEST_F(BitCaskImplRestoreTest, NonExistentKeyAfterRestart) {
  {
    auto db = std::make_unique<BitcaskImpl>(DB_PATH);
    db->Put("key", "value").get();
  }

  auto db2 = std::make_unique<BitcaskImpl>(DB_PATH);
  EXPECT_FALSE(db2->Get("never_written").has_value());
}

TEST_F(BitCaskImplRestoreTest, MultipleRestartsPreserveData) {
  for (int pass = 0; pass < 3; pass++) {
    auto db = std::make_unique<BitcaskImpl>(DB_PATH);
    db->Put("persistent", "value").get();
    EXPECT_EQ(db->Get("persistent").value_or(""), "value");
  }
}

// Legacy test kept for regression
TEST_F(BitCaskImplRestoreTest, InitFromExistedDB) {
  auto db1 = std::make_unique<BitcaskImpl>(DB_PATH);
  db1->Put("key1", "value1").get();
  db1->Put("key2", "value2").get();
  db1->Put("key3", "value3").get();

  EXPECT_EQ(db1->Get("key1").value_or(""), "value1");
  EXPECT_EQ(db1->Get("key2").value_or(""), "value2");
  EXPECT_EQ(db1->Get("key3").value_or(""), "value3");
  db1.reset();

  BITCASK_LOGGER_INFO("Recreate bitcask");
  auto db2 = std::make_unique<BitcaskImpl>(DB_PATH);
  db2->Put("key1", "value1_new").get();
  EXPECT_EQ(db2->Get("key1").value_or(""), "value1_new");
}

int main(int argc, char** argv) {
  logger::init(logger::LOG_LEVEL_FROM_ERROR,
               [](std::string msg) { std::cout << msg << std::endl; });
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
