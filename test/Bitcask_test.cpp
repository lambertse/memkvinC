#include "bitcask/Bitcask.hpp"

#include <gtest/gtest.h>

#include <bitcask/Logger.hpp>
#include <iostream>

#include "bitcask/Define.hpp"

using namespace bitcask;

// Helper: create a fresh DB dir unique to this test run
static std::string makeTmpDir(const std::string& prefix) {
  std::string path =
      "/tmp/" + prefix + "_" +
      std::to_string(
          std::chrono::steady_clock::now().time_since_epoch().count()) +
      "/";
  fs::create_directories(path);
  return path;
}

class BitCaskTest : public ::testing::Test {
 public:
  void SetUp() override {
    _dbDir = makeTmpDir("bitcask_test");
    SetUpDB();
  }
  void TearDown() override {
    delete _db;
    _db = nullptr;
    fs::remove_all(_dbDir);
  }

 protected:
  void SetUpDB() {
    Setting setting;
    setting.SetMaxFileSize(100).SetDbPath(_dbDir);
    _db = Bitcask::Create(setting);
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

TEST_F(BitCaskTest, ReinitDBAndQuery) {
  for (int i = 0; i <= 1000; i++) {
    _db->Put("bitcask", std::to_string(i));
  }

  delete _db;
  _db = nullptr;
  SetUpDB();
  EXPECT_EQ(_db->Get("bitcask"), "1000");
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
    fs::remove_all(_dbDir);
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
  for (auto& entry : fs::directory_iterator(_dbDir))
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

// ── Delete ───────────────────────────────────────────────────────────────────
class BitCaskDeleteTest : public ::testing::Test {
 public:
  void SetUp() override {
    _dbDir = makeTmpDir("bitcask_delete");
    openDB();
  }
  void TearDown() override {
    closeDB();
    fs::remove_all(_dbDir);
  }

 protected:
  void openDB(size_t maxFileSize = 4096) {
    Setting s;
    s.SetDbPath(_dbDir).SetMaxFileSize(maxFileSize);
    _db = Bitcask::Create(s);
  }
  void reopenDB(size_t maxFileSize = 4096) {
    closeDB();
    openDB(maxFileSize);
  }
  void closeDB() {
    delete _db;
    _db = nullptr;
  }

  std::string _dbDir;
  Bitcask* _db = nullptr;
};

// ── Sanity: basic correctness in a single session ────────────────────────────

// After Delete, Get must return nullopt.
TEST_F(BitCaskDeleteTest, DeleteMakesKeyUnavailable) {
  _db->Put("key", "value");
  _db->Delete("key");
  EXPECT_FALSE(_db->Get("key").has_value());
}

// Deleting a key that was never Put must not crash and Get must still return
// nullopt.
TEST_F(BitCaskDeleteTest, DeleteNonExistentKeyIsNoOp) {
  EXPECT_NO_THROW(_db->Delete("ghost"));
  EXPECT_FALSE(_db->Get("ghost").has_value());
}

// After Delete then Put, Get must return the new value, not nullopt.
TEST_F(BitCaskDeleteTest, DeleteThenReputReturnsNewValue) {
  _db->Put("key", "v1");
  _db->Delete("key");
  _db->Put("key", "v2");
  EXPECT_EQ(_db->Get("key").value_or(""), "v2");
}

// Get must never return the raw tombstone sentinel string to the caller
// regardless of what is stored internally.
// Targets BUG-D: CommitWrite stores tombstone string in _activeMap.
TEST_F(BitCaskDeleteTest, TombstoneNotExposedAsValue) {
  _db->Put("key", "value");
  _db->Delete("key");
  auto result = _db->Get("key");
  // Either nullopt (correct) or some real value — but never the sentinel.
  if (result.has_value()) {
    EXPECT_NE(result.value(), std::string(BITCASK_TOMBSTONE_VALUE))
        << "Get() returned the raw tombstone sentinel — internal detail leaked";
  }
}

// Deleting a key twice must not crash and the key must remain unavailable.
TEST_F(BitCaskDeleteTest, DoubleDeleteIsIdempotent) {
  _db->Put("key", "value");
  _db->Delete("key");
  EXPECT_NO_THROW(_db->Delete("key"));
  EXPECT_FALSE(_db->Get("key").has_value());
}

// Only deleted keys disappear; other keys remain intact.
TEST_F(BitCaskDeleteTest, DeleteSubsetLeavesOthersIntact) {
  _db->Put("a", "1");
  _db->Put("b", "2");
  _db->Put("c", "3");
  _db->Delete("a");
  _db->Delete("c");

  EXPECT_FALSE(_db->Get("a").has_value());
  EXPECT_EQ(_db->Get("b").value_or(""), "2");
  EXPECT_FALSE(_db->Get("c").has_value());
}

// ── Persistence: delete must survive a DB restart ────────────────────────────

// Put then Delete then reopen — the key must still be gone.
// The Put and tombstone both land in the active file (no rotation).
TEST_F(BitCaskDeleteTest, DeletePersistsAfterRestart_NoRotation) {
  _db->Put("key", "value");
  _db->Delete("key");
  reopenDB();
  EXPECT_FALSE(_db->Get("key").has_value())
      << "Deleted key reappeared after restart (no rotation). "
         "RestoreActiveMap must erase prior hints when it encounters a "
         "tombstone.";
}

// Same as above but also verify a sibling key is unaffected.
TEST_F(BitCaskDeleteTest, DeletePersistsAfterRestart_SiblingKeyIntact) {
  _db->Put("keep", "good");
  _db->Put("remove", "bad");
  _db->Delete("remove");
  reopenDB();

  EXPECT_EQ(_db->Get("keep").value_or(""), "good");
  EXPECT_FALSE(_db->Get("remove").has_value())
      << "Deleted key reappeared after restart while sibling key survived.";
}

// ── Persistence with file rotation ───────────────────────────────────────────

class BitCaskDeleteRotationTest : public ::testing::Test {
 public:
  void SetUp() override {
    _dbDir = makeTmpDir("bitcask_delete_rot");
    openDB();
  }
  void TearDown() override {
    closeDB();
    fs::remove_all(_dbDir);
  }

 protected:
  void openDB(size_t maxFileSize = 64) {
    Setting s;
    s.SetDbPath(_dbDir).SetMaxFileSize(maxFileSize);
    _db = Bitcask::Create(s);
  }
  void reopenDB(size_t maxFileSize = 64) {
    closeDB();
    openDB(maxFileSize);
  }
  void closeDB() {
    delete _db;
    _db = nullptr;
  }
  // Force at least one file rotation by writing padding records.
  void forceRotation(int n = 15) {
    for (int i = 0; i < n; i++)
      _db->Put("pad" + std::to_string(i), std::string(8, 'x'));
  }

  std::string _dbDir;
  Bitcask* _db = nullptr;
};

// Write target key, force rotation (key lands in stable file), then Delete
// (tombstone in active file).  After restart the key must remain deleted.
//
TEST_F(BitCaskDeleteRotationTest, DeletePersistsAfterRestart_KeyInStableFile) {
  _db->Put("target", "original");
  forceRotation();        // pushes "target" into a stable file
  _db->Delete("target");  // tombstone ends up in the new active file

  reopenDB();
  EXPECT_FALSE(_db->Get("target").has_value())
      << "Deleted key reappeared after restart. "
         "Key was in a stable file; tombstone was in the active file. "
         "Restore must erase prior hints when it encounters a tombstone.";
}

// Write target key, force rotation (key in stable 0), write tombstone and
// force another rotation (tombstone moves to stable 1).  After restart the
// key must remain deleted.
//
TEST_F(BitCaskDeleteRotationTest,
       DeletePersistsAfterRestart_BothInStableFiles) {
  _db->Put("target", "original");
  forceRotation();  // rotate → "target" lands in a stable file

  _db->Delete("target");  // tombstone in new active file
  forceRotation();  // rotate again → tombstone moves to a stable file

  reopenDB();
  EXPECT_FALSE(_db->Get("target").has_value())
      << "Deleted key reappeared after restart. "
         "Both original write and tombstone are in stable files. "
         "RestoreStableMap must erase prior hints when it encounters a "
         "tombstone.";
}

// In-session: write enough to rotate the key to a stable file, then Delete.
// Get must return nullopt in the same session (no restart).
TEST_F(BitCaskDeleteRotationTest, DeleteFromStableFile_SameSession) {
  _db->Put("target", "original");
  forceRotation();
  _db->Delete("target");

  EXPECT_FALSE(_db->Get("target").has_value())
      << "Get returned a value after Delete when key had been rotated to a "
         "stable file.";
}

// Delete a rotated key, then re-Put it.  Must return the new value.
TEST_F(BitCaskDeleteRotationTest, DeleteFromStableFile_ThenReput) {
  _db->Put("target", "v1");
  forceRotation();
  _db->Delete("target");
  _db->Put("target", "v2");

  EXPECT_EQ(_db->Get("target").value_or(""), "v2");
}

// All non-deleted keys survive a rotation + delete + restart cycle.
TEST_F(BitCaskDeleteRotationTest, DeleteWithRotation_SiblingKeysIntact) {
  _db->Put("keep1", "a");
  _db->Put("target", "bad");
  _db->Put("keep2", "b");
  forceRotation();
  _db->Delete("target");

  reopenDB();
  EXPECT_EQ(_db->Get("keep1").value_or(""), "a");
  EXPECT_FALSE(_db->Get("target").has_value());
  EXPECT_EQ(_db->Get("keep2").value_or(""), "b");
}

int main(int argc, char* argv[]) {
  logger::init(logger::LOG_LEVEL_FROM_ERROR,
               [](std::string msg) { std::cout << msg << std::endl; });
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
