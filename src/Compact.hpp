#pragma once

#include <cstddef>
class Compact {
 public:
  Compact(double srcPercentage, size_t srcThreshold);

 private:
  double deadKeyCompactionPercentage;
  size_t deadKeyCompactionSize;
};
