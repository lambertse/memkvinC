#pragma once
#include <cstddef>

#include "bitcask/Type.hpp"
namespace bitcask {

struct Setting {
 public:
  Setting() = default;
  Setting(const fs::path& dbPath, size_t maxFileSize = 1024 * 1024 * 1024,
          double deadKeyCompactionPercentage = 0.6,
          size_t deadKeyCompactionSize = 512 * 1024 * 1024);

  // Getters
  const fs::path& GetDbPath() const;
  size_t GetMaxFileSize() const;
  double GetDeadKeyCompactionPercentage() const;
  size_t GetDeadKeyCompactionSize() const;

  // Setters
  Setting& SetDbPath(const fs::path& path);
  Setting& SetMaxFileSize(size_t size);
  Setting& SetDeadKeyCompactionPercentage(double percentage);
  Setting& SetDeadKeyCompactionSize(size_t size);

 private:
  // clang-format off
  fs::path dbPath;
  size_t maxFileSize;                      // 1GB
  double deadKeyCompactionPercentage;      //  Compact whenever dead key exceed threshold
  size_t deadKeyCompactionSize;         // Compact whenever deadkey size exceed threshold
  // clang-format on
};

}  // namespace bitcask
