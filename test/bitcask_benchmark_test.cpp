#include "bitcask/Bitcask.hpp"
#include "bitcask/Logger.hpp"
#include <filesystem>
#include <gtest/gtest.h>
#include <iostream>

using namespace bitcask;

#define DBPATH "/tmp/bitcask/benchmark/"
class BitcaskBenchmark : public ::testing::Test {
protected:
  void SetUp() override {
    std::filesystem::remove_all(DBPATH);
    std::filesystem::create_directories(DBPATH);
    Setting setting;
    setting.maxFileSize = 1024 * 1024;
    _bitcask = Bitcask::Create(DBPATH, setting);
    ASSERT_NE(_bitcask, nullptr);
  }

  void TearDown() override {
    // std::filesystem::remove_all(DBPATH);
    delete _bitcask;
  }

  Bitcask *_bitcask;
};

TEST_F(BitcaskBenchmark, Put) {
  for (int i = 0; i < 100000; i++) {
    _bitcask->Put(std::to_string(i), std::to_string(i));
  }
  for (int i = 0; i < 100000; i++) {
    auto value = _bitcask->Get(std::to_string(i));
    EXPECT_STREQ(value.value_or("").c_str(), std::to_string(i).c_str());
  }
}

int main(int argc, char **argv) {
  logger::init(logger::LOG_LEVEL_FROM_INFO,
               [](std::string msg) { std::cout << msg << std::endl; });
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
