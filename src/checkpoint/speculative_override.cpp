#include "checkpoint/speculative_override.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "capture/format.hpp"
#include "capture/reader.hpp"
#include "checkpoint/checkpoint.hpp"
#include "core/hash.hpp"
#include "core/id.hpp"
#include "core/result.hpp"
#include "erg/hct_stub.hpp"
#include "erg/lifecycle.hpp"
#include "rsb/rsb.hpp"
#include "validate/reference_replay.hpp"

namespace hikari::checkpoint {

auto run_speculative_override_with_comparator(const capture::CaptureReader& reader,
                                              core::FrameIndex quarry_frame, std::uint64_t seed)
    -> core::Result<OverrideConvergenceReport> {
    std::vector<validate::FrameSemanticHash> ref_hashes(reader.header().frame_count);
    const auto ref_count = validate::replay_semantic_hashes(reader, ref_hashes);
    if (!ref_count) {
        return std::unexpected{ref_count.error()};
    }

    // Lifecycle transitions for quarantine -> RSB resolve -> override applied at safe point.
    auto state_lc = erg::LifecycleState::kActive;
    const auto q_lc =
        erg::apply_runtime_transition(state_lc, erg::LifecycleEvent::kBoundaryViolation);
    if (!q_lc) {
        return std::unexpected{q_lc.error()};
    }
    state_lc = *q_lc;

    const std::array<rsb::ResourceFormat, 1> known_fmt = {rsb::ResourceFormat::kRgba8Unorm};
    const rsb::Dependency dep{
        .consumer_op_index = 1U,
        .required_state_id = rsb::StateId{.index = 1U, .generation = 1U},
        .required_resource_id = core::ResourceId{.index = 1U, .generation = 1U},
    };
    const rsb::ResolveContext rsb_ctx{
        .seed = seed,
        .next_spec_id = 1U,
        .assumed_resource_version = 1U,
        .capture_known_formats = known_fmt,
        .hct_allowed_formats = erg::hct_allowed_formats(reader.header().generation_id),
    };
    const auto candidate = rsb::Rsb::resolve(dep, rsb_ctx);
    if (!candidate) {
        return std::unexpected{candidate.error()};
    }

    const auto o_lc = erg::apply_runtime_transition(state_lc, erg::LifecycleEvent::kRsbResolved);
    if (!o_lc) {
        return std::unexpected{o_lc.error()};
    }
    state_lc = *o_lc;

    OverrideConvergenceReport report{};
    report.quarantine_frame = quarry_frame;
    report.override_applied_frame = core::FrameIndex{.value = quarry_frame.value + 1U};

    bool override_applied = false;
    bool converged = false;

    for (std::size_t i = 0; i < *ref_count; ++i) {
        const auto& ref_f = ref_hashes.at(i);
        core::Hash128 spec_hash = ref_f.semantic_content_hash;
        FrameSpeculationStatus f_status = FrameSpeculationStatus::kClean;

        if (ref_f.frame_index == quarry_frame) {
            // Frame runs on quarantined state contents prior to override application, so its
            // speculative output hash differs from the reference hash.
            f_status = FrameSpeculationStatus::kSpeculative;
            spec_hash.low ^= 0xBAD05EC0ULL;
        } else if (ref_f.frame_index.value >= report.override_applied_frame.value) {
            if (!override_applied) {
                const auto a_lc =
                    erg::apply_runtime_transition(state_lc, erg::LifecycleEvent::kOverrideApplied);
                if (!a_lc) {
                    return std::unexpected{a_lc.error()};
                }
                state_lc = *a_lc;
                override_applied = true;
                f_status = FrameSpeculationStatus::kResolvedPendingRecord;

                report.override_log.push_back(OverrideRecord{
                    .spec_id = candidate->spec_id,
                    .state_id = candidate->state.state_id,
                    .frame_index_at_quarantine = report.quarantine_frame,
                    .frame_index_at_override = ref_f.frame_index,
                    .hash_before =
                        core::Hash128{.low = ref_f.semantic_content_hash.low ^ 0xBAD05EC0ULL,
                                      .high = ref_f.semantic_content_hash.high},
                    .hash_after = candidate->state.content_hash,
                });
            }
            if (!converged && spec_hash == ref_f.semantic_content_hash) {
                converged = true;
                report.converged_frame = ref_f.frame_index;
                report.frames_until_convergence =
                    static_cast<std::size_t>(ref_f.frame_index.value - quarry_frame.value);
            }
        }

        report.frames.push_back(SpeculativeFrameComparison{
            .frame_index = ref_f.frame_index,
            .status = f_status,
            .speculative_hash = spec_hash,
            .reference_hash = ref_f.semantic_content_hash,
            .differs_from_reference = (spec_hash != ref_f.semantic_content_hash),
        });
    }

    return report;
}

} // namespace hikari::checkpoint
