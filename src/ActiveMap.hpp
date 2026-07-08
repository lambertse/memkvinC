#pragma once
#include <libcuckoo/cuckoohash_map.hh>
#include <optional>

#include "bitcask/Define.hpp"

namespace bitcask {

class ActiveMap {
 public:
  ActiveMap();
  ~ActiveMap();

  void Put(const Key& key, const Value& value);
  std::optional<Value> Get(const Key& key);
  void Delete(const Key& key);
  void Clear();

 private:
  libcuckoo::cuckoohash_map<Key, Value> _map;
};
}  // namespace bitcask
