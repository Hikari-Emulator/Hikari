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

// Builds Fixture D: 1-frame capture that clears color_target {1, 1} and then
// emits SampleOp sampling resource R = {3, 1}, which is never written in the capture.
[[nodiscard]] auto build_incomplete_dependency_fixture() -> core::Result<std::vector<std::byte>>;

// Builds Fixture E: same as build_synthetic_fixture() (Fixture A), except Frame 0
// has an extra dead ClearOp on color_target {1, 1} overwritten before any read.
[[nodiscard]] auto build_dead_write_fixture() -> core::Result<std::vector<std::byte>>;

} // namespace hikari::capture
