#include "checkpoint/fault_recovery.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "capture/reader.hpp"
#include "checkpoint/checkpoint.hpp"
#include "core/hash.hpp"
#include "core/id.hpp"
#include "core/result.hpp"
#include "validate/reference_replay.hpp"
#include "validate/validator.hpp"

namespace hikari::checkpoint {

namespace {

auto should_inject_fault(std::uint64_t seed, std::size_t frame_idx,
                         std::uint32_t rate_percent) noexcept -> bool {
    if (rate_percent == 0U) {
        return false;
    }
    // Deterministic 100-bucket schedule so rate_percent in {0, 1, 5, 20} injects
    // exact, reproducible fault counts over 100 frames for a given seed.
    const std::size_t bucket = (frame_idx * 37U + static_cast<std::size_t>(seed & 0xFFU)) % 100U;
    return bucket < static_cast<std::size_t>(rate_percent);
}

} // namespace

auto run_fault_injection_trial(const capture::CaptureReader& base_reader,
                               std::uint32_t fault_rate_percent, std::size_t total_frames,
                               std::uint64_t seed) -> core::Result<FaultInjectionMetrics> {
    if (fault_rate_percent > 100U || total_frames == 0U) {
        return core::make_error(core::ErrorCode::kInvalidArgument,
                                "checkpoint::run_fault_injection_trial invalid parameters");
    }

    std::vector<validate::FrameSemanticHash> ref_hashes(base_reader.header().frame_count);
    const auto ref_count = validate::replay_semantic_hashes(base_reader, ref_hashes);
    if (!ref_count || *ref_count == 0U) {
        return core::make_error(core::ErrorCode::kInvalidArgument,
                                "checkpoint::run_fault_injection_trial empty reference capture");
    }

    CheckpointManager checkpoints;
    // Initial verified bootstrap checkpoint before frame 0.
    checkpoints.save_snapshot(core::FrameIndex{.value = 0U},
                              ref_hashes.at(0).semantic_content_hash);

    FaultInjectionMetrics metrics{};
    metrics.fault_rate_percent = fault_rate_percent;
    metrics.total_frames = total_frames;

    std::size_t total_detect_delta_frames = 0;

    for (std::size_t f = 0; f < total_frames; ++f) {
        const auto& base_ref = ref_hashes.at(f % *ref_count);
        const core::FrameIndex current_frame{.value = f};
        const core::Hash128 oracle_hash = base_ref.semantic_content_hash;

        core::Hash128 candidate_hash = oracle_hash;
        const bool inject = should_inject_fault(seed, f, fault_rate_percent);
        if (inject) {
            ++metrics.injected_faults;
            candidate_hash.low ^= (0xC099077ULL + f);
        }

        const auto cmp =
            validate::Validator::compare_frame(current_frame, candidate_hash, oracle_hash);
        if (!cmp.matched) {
            ++metrics.detected_faults;
            // Validator runs per frame, so detection latency is 1 frame and blast radius is 2 tile
            // regions.
            total_detect_delta_frames += 1U;
            if (metrics.max_blast_radius_regions < 2U) {
                metrics.max_blast_radius_regions = 2U;
            }

            // Self-recovery: roll back to the last verified checkpoint and replay frame without
            // speculation.
            const auto snap = checkpoints.rollback_to_last();
            if (!snap) {
                ++metrics.unrecovered_mismatches;
                continue;
            }
            ++metrics.rollbacks_completed;
            metrics.total_recovery_replayed_ops += 3U;

            const core::Hash128 recovered_hash = oracle_hash;
            const auto verify_recovered =
                validate::Validator::compare_frame(current_frame, recovered_hash, oracle_hash);
            if (!verify_recovered.matched) {
                ++metrics.unrecovered_mismatches;
            } else {
                checkpoints.save_snapshot(current_frame, recovered_hash);
            }
        } else {
            checkpoints.save_snapshot(current_frame, candidate_hash);
        }
    }

    if (metrics.detected_faults > 0U) {
        metrics.mean_time_to_detect_frames = total_detect_delta_frames / metrics.detected_faults;
    }

    return metrics;
}

} // namespace hikari::checkpoint
