#include "bitcask/Bitcask.hpp"

#include "BitcaskImpl.hpp"

namespace bitcask {
Bitcask* Bitcask::Create(const std::string& db_path, const Setting& setting) {
  return new Bitcask(db_path, setting);
}

void Bitcask::Put(const Key& key, const Value& value) {
  return _impl->Put(key, value).get();
}

std::future<void> Bitcask::PutAsync(const Key& key, const Value& value) {
  return _impl->Put(key, value);
}

std::optional<Value> Bitcask::Get(const Key& key) { return _impl->Get(key); }

bool Bitcask::Delete(const Key& key) { return _impl->Delete(key); }

Bitcask::Bitcask(const std::string& db_path, const Setting& setting) {
  _impl = std::make_shared<BitcaskImpl>(db_path, setting);
}

Bitcask::~Bitcask() {}

}  // namespace bitcask
