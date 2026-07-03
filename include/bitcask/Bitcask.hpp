#pragma once
#include <future>
#include <memory>
#include <optional>

#include "LibExport.hpp"
#include "Setting.hpp"
#include "Type.hpp"
namespace bitcask {
class Bitcask {
 public:
  static BITCASK_EXPORT Bitcask* Create(const std::string& dbDir,
                                        const Setting& setting = Setting{});
  ~Bitcask();

  BITCASK_EXPORT void Put(const Key& key, const Value& value);
  BITCASK_EXPORT std::future<void> PutAsync(const Key& key, const Value& value);

  BITCASK_EXPORT std::optional<Value> Get(const Key& key);
  BITCASK_EXPORT bool Delete(const Key& key);

 private:
  Bitcask(const std::string& dbDir, const Setting& setting = Setting{});
  Bitcask(const Bitcask&);
  Bitcask& operator=(const Bitcask&);

  std::shared_ptr<class BitcaskImpl> _impl;
};

}  // namespace bitcask
