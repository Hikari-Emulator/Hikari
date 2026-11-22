#pragma once

#include <cstddef>
#include <vector>

#include "core/result.hpp"

namespace hikari::capture {

// Builds a deterministic 3-frame synthetic capture with hand-written clear
// and draw-pattern operations (no game data).
[[nodiscard]] auto build_synthetic_fixture() -> core::Result<std::vector<std::byte>>;

} // namespace hikari::capture
