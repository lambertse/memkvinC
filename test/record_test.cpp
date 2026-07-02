#include "Record.hpp"
#include "bitcask/Logger.hpp"
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <vector>

using namespace bitcask;
using namespace file;

class RecordTest : public ::testing::Test {
protected:
  void SetUp() override {
    testFile = new std::fstream();
    testFile->open("/tmp/testfile.dat", std::ios::binary | std::ios::out |
                                            std::ios::in | std::ios::trunc);
    ASSERT_TRUE(testFile->is_open());
  }

  void TearDown() override {
    testFile->close();
    delete testFile;
    std::remove("/tmp/testfile.dat");
  }

  void flushAndRewind() {
    testFile->flush();
    testFile->seekg(0, std::ios::beg);
  }

  FileHandler testFile;
};

TEST_F(RecordTest, WriteAndReadSingleRecord) {
  Key key = "hello";
  Value value = "world";
  WriteRecord(testFile, key, value);

  std::vector<std::pair<Key, Value>> results;
  ReadAllRecordFromFile(testFile, [&](const Key &k, const Value &v, RecordInf) {
    results.push_back({k, v});
  });

  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0].first, key);
  EXPECT_EQ(results[0].second, value);
}

TEST_F(RecordTest, WriteAndReadMultipleRecordsInOrder) {
  std::vector<std::pair<Key, Value>> written = {
      {"key1", "value1"}, {"key2", "value2"}, {"key3", "value3"}};
  for (auto &[k, v] : written)
    WriteRecord(testFile, k, v);

  std::vector<std::pair<Key, Value>> results;
  ReadAllRecordFromFile(testFile, [&](const Key &k, const Value &v, RecordInf) {
    results.push_back({k, v});
  });

  ASSERT_EQ(results.size(), written.size());
  for (size_t i = 0; i < written.size(); i++) {
    EXPECT_EQ(results[i].first, written[i].first);
    EXPECT_EQ(results[i].second, written[i].second);
  }
}

TEST_F(RecordTest, EmptyFileReadsNoRecords) {
  int callCount = 0;
  ReadAllRecordFromFile(testFile,
                        [&](const Key &, const Value &, RecordInf) {
                          callCount++;
                        });
  EXPECT_EQ(callCount, 0);
}

TEST_F(RecordTest, RecordInfValueOffsetIsNonZero) {
  Key key = "k";
  Value value = "v";
  RecordInf inf = WriteRecord(testFile, key, value);

  // valueOffset must come after the header and key bytes
  EXPECT_GT(inf.valueOffset, 0u);
  EXPECT_GT(inf.valueOffset, inf.keyOffset);
}

TEST_F(RecordTest, ReadAllRecordFromFileTest) {
  Key key = "anhhai";
  Value value = "trile";
  WriteRecord(testFile, key, value);

  key = "anhba";
  value = "khoale";
  WriteRecord(testFile, key, value);

  key = "abcdefgh";
  value = "111111111111111";
  WriteRecord(testFile, key, value);

  std::vector<std::pair<Key, Value>> results;
  ReadAllRecordFromFile(testFile, [&](const Key &k, const Value &v, RecordInf) {
    results.push_back({k, v});
  });

  ASSERT_EQ(results.size(), 3u);
  EXPECT_EQ(results[0].first, "anhhai");
  EXPECT_EQ(results[0].second, "trile");
  EXPECT_EQ(results[1].first, "anhba");
  EXPECT_EQ(results[1].second, "khoale");
  EXPECT_EQ(results[2].first, "abcdefgh");
  EXPECT_EQ(results[2].second, "111111111111111");
}

int main(int argc, char **argv) {
  logger::init(logger::LOG_LEVEL_FROM_ERROR,
               [](std::string msg) { std::cout << msg << std::endl; });
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
