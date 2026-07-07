#include "bitcask/Setting.hpp"

namespace bitcask {

namespace {
namespace Internal {

std::string ensureEndingSlash(const std::string& path);

}  // namespace Internal
}  // namespace

Setting::Setting(const std::filesystem::path& dbPath, size_t maxFileSize,
                 double deadKeyCompactionPercentage,
                 size_t deadKeyCompactionSize)
    : dbPath(dbPath),
      maxFileSize(maxFileSize),
      deadKeyCompactionPercentage(deadKeyCompactionPercentage),
      deadKeyCompactionSize(deadKeyCompactionSize) {}

// Getters
const std::filesystem::path& Setting::GetDbPath() const { return dbPath; }
size_t Setting::GetMaxFileSize() const { return maxFileSize; }

double Setting::GetDeadKeyCompactionPercentage() const {
  return deadKeyCompactionPercentage;
}

size_t Setting::GetDeadKeyCompactionSize() const {
  return deadKeyCompactionSize;
}

// Setters
Setting& Setting::SetDbPath(const std::filesystem::path& path) {
  dbPath = Internal::ensureEndingSlash(path.string());
  return *this;
}

Setting& Setting::SetMaxFileSize(size_t size) {
  maxFileSize = size;
  return *this;
}

Setting& Setting::SetDeadKeyCompactionPercentage(double percentage) {
  deadKeyCompactionPercentage = percentage;
  return *this;
}

Setting& Setting::SetDeadKeyCompactionSize(size_t size) {
  deadKeyCompactionSize = size;
  return *this;
}

namespace {
namespace Internal {

std::string ensureEndingSlash(const std::string& path) {
  if (path.empty() || path.back() == '/') {
    return path;
  }
  return path + "/";
}

}  // namespace Internal
}  // namespace

}  // namespace bitcask
