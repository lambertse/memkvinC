#include "ActiveFile.hpp"

#include "File.hpp"
#include "Record.hpp"
#include "bitcask/Logger.hpp"
#include "bitcask/Define.hpp"

namespace bitcask {
using namespace file;
ActiveFile::~ActiveFile() {
  if (_file) {
    _file->flush();
    _file->close();
    delete _file;
  }
}
FileHandler ActiveFile::Restore(const std::string& filename,
                                const RecordFoundCallback& callback) {
  if (!fs::exists(filename)) {
    BITCASK_LOGGER_INFO("File does not exist: {}", filename);
    return nullptr;
  }

  // First open file in read mode to process existing records
  FileHandler read = new std::fstream();
  if (!OpenFile(read, filename, std::ios::binary | std::ios::in)) {
    BITCASK_LOGGER_ERROR("Failed to open file for reading: {}", filename);
    return nullptr;
  }

  // Process all records
  ReadAllRecordFromFile(read, callback);
  if (read) {
    read->close();
    delete read;
  }
  // Reopen in append mode for future writes
  FileHandler result = new std::fstream();
  if (!OpenFile(
          result, filename,
          std::ios::binary | std::ios::in | std::ios::out | std::ios::app)) {
    BITCASK_LOGGER_ERROR("Failed to open file for appending: {}", filename);
    delete result;  // Clean up on failure
    return nullptr;
  }

  BITCASK_LOGGER_INFO("Successfully restored file: {}", filename);
  return result;
}

ActiveFile::Handler ActiveFile::Create(file::FileHandler file) {
  return std::make_shared<ActiveFile>(file);
}
ActiveFile::Handler ActiveFile::Create(const std::string& path) {
  FileHandler file = new std::fstream();
  if (!OpenFile(
          file, path,
          std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc)) {
    BITCASK_LOGGER_ERROR("Cannot create active file: {}", path);
    return nullptr;
  }
  return Create(file);
}

Offset ActiveFile::Write(const Key& key, const Value& value) {
  return WriteRecord(_file, key, value).valueOffset;
}

FileHandler ActiveFile::Rotate() {
  _file->flush();
  _file->close();
  FileHandler newFile = new std::fstream();
  swap(_file, newFile);
  return newFile;
}
}  // namespace bitcask
