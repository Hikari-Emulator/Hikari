#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "capture/reader.hpp"
#include "checkpoint/checkpoint.hpp"
#include "core/hash.hpp"
#include "core/id.hpp"
#include "core/result.hpp"

namespace hikari::checkpoint {

enum class FrameSpeculationStatus : std::uint32_t {
    kClean = 1,
    kSpeculative = 2,
    kResolvedPendingRecord = 3,
};

struct SpeculativeFrameComparison {
    core::FrameIndex frame_index{};
    FrameSpeculationStatus status = FrameSpeculationStatus::kClean;
    core::Hash128 speculative_hash{};
    core::Hash128 reference_hash{};
    bool differs_from_reference = false;
};

struct OverrideConvergenceReport {
    std::vector<SpeculativeFrameComparison> frames;
    std::vector<OverrideRecord> override_log;
    core::FrameIndex quarantine_frame{};
    core::FrameIndex override_applied_frame{};
    core::FrameIndex converged_frame{};
    std::size_t frames_until_convergence = 0;
};

// Simulates boundary quarantine on quarry_frame, continues execution on quarantined state
// contents while marking quarry_frame SPECULATIVE, applies the RSB candidate override at the
// next-frame boundary safe point (quarry_frame + 1), records the OverrideRecord, and compares
// every frame against the reference path.
[[nodiscard]] auto run_speculative_override_with_comparator(const capture::CaptureReader& reader,
                                                            core::FrameIndex quarry_frame,
                                                            std::uint64_t seed = 0x5EED1234ULL)
    -> core::Result<OverrideConvergenceReport>;

} // namespace hikari::checkpoint
