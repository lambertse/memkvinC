#include "ActiveMap.hpp"

#include "bitcask/Logger.hpp"

namespace bitcask {
ActiveMap::ActiveMap() = default;

ActiveMap::~ActiveMap() = default;

std::optional<Value> ActiveMap::Get(const Key& key) {
  Value value;
  if (_map.find(key, value)) {
    return value;
  }
  return std::nullopt;
}
void ActiveMap::Put(const Key& key, const Value& value) {
  _map.insert_or_assign(key, value);
}
void ActiveMap::Delete(const Key& key) { _map.erase(key); }

void ActiveMap::Clear() { _map.clear(); }

}  // namespace bitcask
