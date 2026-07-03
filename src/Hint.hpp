#pragma once
#include <stdint.h>
namespace bitcask {
struct Hint {
  uint32_t fd;      // file descriptor
  uint32_t offset;  // offset
  uint32_t size;    // size
  Hint() : fd(0), offset(0), size(0) {}

  Hint(uint32_t fd, uint32_t offset, uint32_t size)
      : fd(fd), offset(offset), size(size) {}
};

}  // namespace bitcask
