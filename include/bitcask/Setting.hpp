#pragma once
#include <cstddef>
#include <filesystem>
namespace bitcask {

struct Setting {
 public:
  Setting() = default;
  Setting(const std::filesystem::path& dbPath, size_t maxFileSize,
          double deadKeyCompactionPercentage, size_t deadKeyCompactionSize);

  // Getters
  const std::filesystem::path& GetDbPath() const;
  size_t GetMaxFileSize() const;
  double GetDeadKeyCompactionPercentage() const;
  size_t GetDeadKeyCompactionSize() const;

  // Setters
  Setting& SetDbPath(const std::filesystem::path& path);
  Setting& SetMaxFileSize(size_t size);
  Setting& SetDeadKeyCompactionPercentage(double percentage);
  Setting& SetDeadKeyCompactionSize(size_t size);

 private:
  // clang-format off
  std::filesystem::path dbPath;
  size_t maxFileSize = 1024 * 1024 * 1024;                      // 1GB
  double deadKeyCompactionPercentage = 0.6;                     // Compact whenever dead key exceed threshold
  size_t deadKeyCompactionSize = 512 * 1024 * 1024;             // Compact whenever deadkey size exceed threshold
  // clang-format on
};

}  // namespace bitcask
