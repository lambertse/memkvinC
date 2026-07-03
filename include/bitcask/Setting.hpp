#pragma once
#include <cstddef>
namespace bitcask {

struct Setting {
  size_t maxFileSize = 1024 * 1024 * 1024;  // 1GB
};

}  // namespace bitcask
