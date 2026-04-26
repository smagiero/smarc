// **********************************************************************
// smesh/include/SmeshTypes.hpp
// **********************************************************************
// Sebastian Claudiusz Magierowski Apr 26 2026
#pragma once

#include <cstddef>
#include <cstdint>

namespace smesh {

constexpr std::size_t kDim = 4;
constexpr std::size_t kScratchpadRows = 16;
constexpr std::size_t kAccumulatorRows = 16;

using Elem = std::int8_t;
using Acc = std::int32_t;

struct MatrixShape {
  std::size_t rows = 0;
  std::size_t cols = 0;
};

} // namespace smesh
