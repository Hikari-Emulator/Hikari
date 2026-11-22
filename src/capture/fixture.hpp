#pragma once

#include <cstddef>
#include <vector>

#include "core/result.hpp"

namespace hikari::capture {

// Builds a deterministic 3-frame synthetic capture with hand-written clear
// and draw-pattern operations (no game data).
[[nodiscard]] auto build_synthetic_fixture() -> core::Result<std::vector<std::byte>>;

// Builds Fixture B: same operations as build_synthetic_fixture(), except in
// Frame 2 the barrier/order between the vertex_buf pattern op and color_target
// read/draw op is moved from before the second op to after it.
[[nodiscard]] auto build_barrier_reordered_fixture() -> core::Result<std::vector<std::byte>>;

// Builds Fixture C: same as build_synthetic_fixture(), except Frame 1 contains
// zero operations between BeginFrame(1) and EndFrame(1).
[[nodiscard]] auto build_empty_frame_fixture() -> core::Result<std::vector<std::byte>>;

} // namespace hikari::capture
