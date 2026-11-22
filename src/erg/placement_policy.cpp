#include "erg/placement_policy.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

#include "capture/reader.hpp"
#include "core/result.hpp"
#include "erg/erg.hpp"
#include "erg/lifecycle.hpp"
#include "validate/rsb_erg_replay.hpp"

namespace hikari::erg {

auto replay_with_placement_policy(const capture::CaptureReader& reader,
                                  const RelevancePlacementPolicy& policy, std::uint64_t seed)
    -> core::Result<std::vector<PlacementFrameSummary>> {
    const auto rsb_erg = validate::replay_rsb_erg_hashes(reader, seed);
    if (!rsb_erg) {
        return std::unexpected{rsb_erg.error()};
    }

    std::vector<PlacementFrameSummary> summaries;
    summaries.reserve(rsb_erg->frames.size());

    for (const auto& frame : rsb_erg->frames) {
        std::size_t active_count = 0;
        std::size_t cached_count = 0;
        std::size_t reactivated_count = 0;

        for (std::size_t i = 0; i < frame.nodes_after_liveness; ++i) {
            ErgNode synthetic_node{};
            synthetic_node.hotness = (i % 2U == 0U) ? 10 : 20;
            synthetic_node.speculative = (i < frame.speculative_candidates_resolved);

            const LifecycleState target_state = policy.decide_placement(synthetic_node);
            if (target_state == LifecycleState::kCached) {
                ++cached_count;
                // Reactivate on frame demand via REQUEST_HIT (guard pass) and verify transition.
                const auto reactivated = apply_runtime_transition(LifecycleState::kCached,
                                                                  LifecycleEvent::kRequestHitPass);
                if (!reactivated || *reactivated != LifecycleState::kActive) {
                    return core::make_error(
                        core::ErrorCode::kInvalidArgument,
                        "erg::replay_with_placement_policy reactivation failed");
                }
                ++reactivated_count;
            } else {
                ++active_count;
            }
        }

        summaries.push_back(PlacementFrameSummary{
            .frame_hash = frame.frame_hash,
            .active_count = active_count,
            .cached_count = cached_count,
            .reactivated_count = reactivated_count,
        });
    }

    return summaries;
}

} // namespace hikari::erg
