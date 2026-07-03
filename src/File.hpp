#pragma once

#include <fstream>

#include "bitcask/Type.hpp"

namespace bitcask::file {

using FileHandler = std::fstream*;
bool OpenFile(FileHandler& file, const std::string& filename,
              std::ios::openmode mode);
bool ReadFile(FileHandler& file, void* buf, size_t size);
std::string ReadValueFromFile(FileHandler& file, size_t size);

std::string ReadFile(FileHandler& file, Offset offset, size_t size);
long WriteFile(FileHandler& file, const void* buf, size_t size);
bool Exist(const std::string& file);

}  // namespace bitcask::file
