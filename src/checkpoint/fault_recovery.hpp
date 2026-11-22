#pragma once

#include <cstddef>
#include <cstdint>

#include "capture/reader.hpp"
#include "core/result.hpp"

namespace hikari::checkpoint {

struct FaultInjectionMetrics {
    std::uint32_t fault_rate_percent = 0;
    std::size_t total_frames = 0;
    std::size_t injected_faults = 0;
    std::size_t detected_faults = 0;
    std::size_t rollbacks_completed = 0;
    std::size_t unrecovered_mismatches = 0;
    std::size_t mean_time_to_detect_frames = 0;
    std::size_t max_blast_radius_regions = 0;
    std::size_t total_recovery_replayed_ops = 0;
};

// Runs a 100-frame synthetic replay loop with deterministic fault injection at
// fault_rate_percent (0..100). On every validator mismatch against the reference path,
// rolls back to the last verified checkpoint and replays with non-speculative state.
[[nodiscard]] auto run_fault_injection_trial(const capture::CaptureReader& base_reader,
                                             std::uint32_t fault_rate_percent,
                                             std::size_t total_frames = 100U,
                                             std::uint64_t seed = 0xFA0175EEDULL)
    -> core::Result<FaultInjectionMetrics>;

} // namespace hikari::checkpoint
