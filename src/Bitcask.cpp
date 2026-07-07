#include "bitcask/Bitcask.hpp"

#include "BitcaskImpl.hpp"

namespace bitcask {
Bitcask* Bitcask::Create(const Setting& setting) {
  return new Bitcask(setting);
}

void Bitcask::Put(const Key& key, const Value& value) {
  return _impl->Put(key, value).get();
}

std::future<void> Bitcask::PutAsync(const Key& key, const Value& value) {
  return _impl->Put(key, value);
}

std::optional<Value> Bitcask::Get(const Key& key) { return _impl->Get(key); }

bool Bitcask::Delete(const Key& key) { return _impl->Delete(key); }

Bitcask::Bitcask(const Setting& setting) {
  _impl = std::make_shared<BitcaskImpl>(setting);
}

Bitcask::~Bitcask() {}

}  // namespace bitcask
