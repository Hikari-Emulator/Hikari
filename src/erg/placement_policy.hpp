#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "capture/reader.hpp"
#include "core/result.hpp"
#include "erg/erg.hpp"
#include "erg/lifecycle.hpp"
#include "validate/reference_replay.hpp"

namespace hikari::erg {

class RelevancePlacementPolicy {
public:
    virtual ~RelevancePlacementPolicy() noexcept = default;
    RelevancePlacementPolicy() noexcept = default;
    RelevancePlacementPolicy(const RelevancePlacementPolicy&) noexcept = default;
    auto operator=(const RelevancePlacementPolicy&) noexcept -> RelevancePlacementPolicy& = default;
    RelevancePlacementPolicy(RelevancePlacementPolicy&&) noexcept = default;
    auto operator=(RelevancePlacementPolicy&&) noexcept -> RelevancePlacementPolicy& = default;

    [[nodiscard]] virtual auto decide_placement(const ErgNode& node) const noexcept
        -> LifecycleState = 0;
};

class AlwaysActivePlacementPolicy final : public RelevancePlacementPolicy {
public:
    [[nodiscard]] auto decide_placement(const ErgNode& /*node*/) const noexcept
        -> LifecycleState override {
        return LifecycleState::kActive;
    }
};

class ThresholdPlacementPolicy final : public RelevancePlacementPolicy {
public:
    explicit constexpr ThresholdPlacementPolicy(std::int64_t min_active_hotness) noexcept
        : min_active_hotness_(min_active_hotness) {}

    [[nodiscard]] auto decide_placement(const ErgNode& node) const noexcept
        -> LifecycleState override {
        if (!node.speculative && node.hotness < min_active_hotness_) {
            return LifecycleState::kCached;
        }
        return LifecycleState::kActive;
    }

private:
    std::int64_t min_active_hotness_ = 15;
};

struct PlacementFrameSummary {
    validate::FrameSemanticHash frame_hash{};
    std::size_t active_count = 0;
    std::size_t cached_count = 0;
    std::size_t reactivated_count = 0;
};

[[nodiscard]] auto replay_with_placement_policy(const capture::CaptureReader& reader,
                                                const RelevancePlacementPolicy& policy,
                                                std::uint64_t seed = 0x5EED1234ULL)
    -> core::Result<std::vector<PlacementFrameSummary>>;

} // namespace hikari::erg
