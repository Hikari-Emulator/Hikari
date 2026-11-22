#pragma once

#include <cstddef>
#include <span>

#include "core/hash.hpp"
#include "core/id.hpp"
#include "core/result.hpp"

namespace hikari::validate {

// Per-frame XXH3-128 hash of the decoded semantic operation stream.
// Phase 0 note: this is NOT a rendered-pixel hash.
struct FrameSemanticHash {
    core::FrameIndex frame_index{};
    core::Hash128 semantic_content_hash{};
};

// Replays a validated capture v1 buffer into caller-provided storage with zero
// heap allocations in the replay loop. Returns the number of frames written.
[[nodiscard]] auto replay_semantic_hashes(std::span<const std::byte> capture_bytes,
                                          std::span<FrameSemanticHash> out_frames) noexcept
    -> core::Result<std::size_t>;

} // namespace hikari::validate
