#pragma once

#include <cstddef>

#include "bitcask/Define.hpp"
namespace bitcask {
class Compact {
 public:
  Compact(const fs::path& dbPath);

 private:
  fs::path _dbPath;
};
}  // namespace bitcask
