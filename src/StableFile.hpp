#pragma once
#include <memory>

#include "File.hpp"
#include "Record.hpp"
#include "bitcask/Type.hpp"

namespace bitcask {
class StableFile {
 public:
  using Handler = std::shared_ptr<StableFile>;
  explicit StableFile(file::FileHandler file) : _file(file) {}
  virtual ~StableFile();

  static file::FileHandler Restore(const std::string& filename,
                                   const RecordFoundCallback& callback);
  static Handler Create(file::FileHandler file);
  Value Read(const Key& key, Offset offset, size_t size);

 protected:
  file::FileHandler _file;
};
}  // namespace bitcask
