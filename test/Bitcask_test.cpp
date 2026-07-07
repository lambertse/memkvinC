#include "bitcask/Bitcask.hpp"

#include <gtest/gtest.h>

#include <bitcask/Logger.hpp>
#include <filesystem>
#include <iostream>

using namespace bitcask;

// Helper: create a fresh DB dir unique to this test run
static std::string makeTmpDir(const std::string& prefix) {
  std::string path =
      "/tmp/" + prefix + "_" +
      std::to_string(
          std::chrono::steady_clock::now().time_since_epoch().count()) +
      "/";
  std::filesystem::create_directories(path);
  return path;
}

class BitCaskTest : public ::testing::Test {
 public:
  void SetUp() override {
    _dbDir = makeTmpDir("bitcask_test");
    _db = Bitcask::Create(Setting(_dbDir));
  }
  void TearDown() override {
    delete _db;
    _db = nullptr;
    std::filesystem::remove_all(_dbDir);
  }

 protected:
  std::string _dbDir;
  Bitcask* _db = nullptr;
};

// ── Basic Put / Get ──────────────────────────────────────────────────────────

TEST_F(BitCaskTest, PutAndGet) {
  _db->PutAsync("key1", "value1");
  _db->PutAsync("key2", "value2");
  _db->Put("key3", "value3");  // synchronous: blocks until committed

  EXPECT_EQ(_db->Get("key1").value_or(""), "value1");
  EXPECT_EQ(_db->Get("key2").value_or(""), "value2");
  EXPECT_EQ(_db->Get("key3").value_or(""), "value3");
}

TEST_F(BitCaskTest, GetNonExistentKeyReturnsNullopt) {
  EXPECT_FALSE(_db->Get("missing_key").has_value());
}

TEST_F(BitCaskTest, OverwriteKeyReturnsLatestValue) {
  _db->Put("key", "first");
  _db->Put("key", "second");

  EXPECT_EQ(_db->Get("key").value_or(""), "second");
}

TEST_F(BitCaskTest, PutAsyncWaitForFutureThenGet) {
  auto f1 = _db->PutAsync("k1", "v1");
  auto f2 = _db->PutAsync("k2", "v2");
  f1.get();
  f2.get();

  EXPECT_EQ(_db->Get("k1").value_or(""), "v1");
  EXPECT_EQ(_db->Get("k2").value_or(""), "v2");
}

TEST_F(BitCaskTest, PutAsyncBatchAllCommittedBeforeLastFutureResolves) {
  // Queue multiple writes without waiting; the last .get() guarantees all
  // preceding writes in the same batch are also committed.
  auto f1 = _db->PutAsync("a", "1");
  auto f2 = _db->PutAsync("b", "2");
  auto f3 = _db->PutAsync("c", "3");
  f1.get();
  f2.get();
  f3.get();

  EXPECT_EQ(_db->Get("a").value_or(""), "1");
  EXPECT_EQ(_db->Get("b").value_or(""), "2");
  EXPECT_EQ(_db->Get("c").value_or(""), "3");
}

TEST_F(BitCaskTest, PutManyKeys) {
  const int n = 50;
  for (int i = 0; i < n; i++)
    _db->Put(std::to_string(i), "val" + std::to_string(i));

  for (int i = 0; i < n; i++)
    EXPECT_EQ(_db->Get(std::to_string(i)).value_or(""),
              "val" + std::to_string(i));
}

TEST_F(BitCaskTest, EmptyValue) {
  _db->Put("key", "");
  auto result = _db->Get("key");
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), "");
}

TEST_F(BitCaskTest, LargeValue) {
  std::string largeVal(4096, 'x');
  _db->Put("bigkey", largeVal);
  EXPECT_EQ(_db->Get("bigkey").value_or(""), largeVal);
}

TEST_F(BitCaskTest, DeleteReturnsFalse) {
  _db->Put("key", "value");
  EXPECT_FALSE(_db->Delete("key"));
}

// ── File rotation ────────────────────────────────────────────────────────────
// Use a very small maxFileSize to force rotation after a few records.

class BitCaskRotationTest : public ::testing::Test {
 public:
  void SetUp() override {
    _dbDir = makeTmpDir("bitcask_rotation");
    Setting s;
    s.SetDbPath(_dbDir).SetMaxFileSize(128);
    _db = Bitcask::Create(s);
  }
  void TearDown() override {
    delete _db;
    _db = nullptr;
    std::filesystem::remove_all(_dbDir);
  }

 protected:
  std::string _dbDir;
  Bitcask* _db = nullptr;
};

TEST_F(BitCaskRotationTest, KeysWrittenBeforeRotationAreReadable) {
  // Write enough records to guarantee at least one rotation
  for (int i = 0; i < 20; i++)
    _db->Put("key" + std::to_string(i), "value" + std::to_string(i));

  for (int i = 0; i < 20; i++)
    EXPECT_EQ(_db->Get("key" + std::to_string(i)).value_or(""),
              "value" + std::to_string(i))
        << "Failed for key" << i;
}

TEST_F(BitCaskRotationTest, MultipleStableFilesCreated) {
  for (int i = 0; i < 20; i++)
    _db->Put("k" + std::to_string(i), "v" + std::to_string(i));

  // At least one .db file beyond the active file should exist
  int dbFileCount = 0;
  for (auto& entry : std::filesystem::directory_iterator(_dbDir))
    if (entry.path().extension() == ".db") dbFileCount++;

  EXPECT_GT(dbFileCount, 1) << "Expected rotation to create multiple .db files";
}

TEST_F(BitCaskRotationTest, DataPersistsAfterRestartWithRotation) {
  for (int i = 0; i < 20; i++)
    _db->Put("key" + std::to_string(i), "value" + std::to_string(i));
  delete _db;
  _db = nullptr;

  // Reopen
  Setting s;
  s.SetDbPath(_dbDir).SetMaxFileSize(128);
  _db = Bitcask::Create(s);

  for (int i = 0; i < 20; i++)
    EXPECT_EQ(_db->Get("key" + std::to_string(i)).value_or(""),
              "value" + std::to_string(i))
        << "Failed after restart for key" << i;
}

int main(int argc, char* argv[]) {
  logger::init(logger::LOG_LEVEL_FROM_ERROR,
               [](std::string msg) { std::cout << msg << std::endl; });
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
