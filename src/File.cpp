#include "File.hpp"

#include "bitcask/Logger.hpp"
#include "bitcask/Define.hpp"

namespace bitcask::file {

bool OpenFile(FileHandler& file, const std::string& filename,
              std::ios::openmode mode) {
  file->open(filename, mode);
  BITCASK_LOGGER_INFO("Open file: {}, cur pos: {}", filename, file->tellp());
  return file->is_open();
}

bool ReadFile(FileHandler& file, void* buf, size_t size) {
  if (!file->read(static_cast<char*>(buf), size)) {
    return false;
  }
  return true;
}

std::string ReadValueFromFile(FileHandler& file, size_t size) {
  std::string buf(size, '\0');
  if (!ReadFile(file, &buf[0], size)) {
    return "";
  }
  return buf;
}

std::string ReadFile(FileHandler& file, size_t size) {
  std::string buf(size, '\0');
  if (!ReadFile(file, &buf[0], size)) {
    return "";
  }
  return buf;
}

std::string ReadFile(FileHandler& file, Offset offset, size_t size) {
  file->seekg(offset, std::ios::beg);
  return ReadFile(file, size);
}

long WriteFile(FileHandler& file, const void* buf, size_t size) {
  file->write(static_cast<const char*>(buf), size);
  return static_cast<long>(file->tellp());
}

bool Exist(const std::string& file) {
  std::error_code ec;
  return std::filesystem::exists(file, ec);
}
}  // namespace bitcask::file
