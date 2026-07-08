#pragma once
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
namespace bitcask {
using Key = std::string;
using Value = std::string;
using Offset = uint32_t;

using Map = std::map<Key, Value>;
namespace fs = std::filesystem;
}  // namespace bitcask

#define BITCASK_TOMBSTONE_VALUE "__BITCASK_TOMBSTONE__"
