#pragma once
#include <memory>

#include "Record.hpp"
#include "bitcask/Define.hpp"

namespace bitcask {

class ActiveFile {
 public:
  using Handler = std::shared_ptr<ActiveFile>;

  ActiveFile(file::FileHandler file) : _file(file) {}
  virtual ~ActiveFile();
  static file::FileHandler Restore(const std::string& filename,
                                   const RecordFoundCallback& callback);

  static Handler Create(file::FileHandler file);
  static Handler Create(const std::string& path);
  Offset Write(const Key& key, const Value& value);

  file::FileHandler Rotate();

 protected:
  file::FileHandler _file;
};

}  // namespace bitcask
