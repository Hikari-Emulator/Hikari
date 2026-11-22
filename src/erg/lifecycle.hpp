#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "core/result.hpp"
#include "rsb/rsb.hpp"

namespace hikari::erg {

enum class LifecycleState : std::uint32_t {
    kActive = 0,
    kCached = 1,
    kStale = 2,
    kQuarantined = 3,
    kOverridden = 4,
};

enum class LifecycleEvent : std::uint32_t {
    kPlacementEvict = 0,
    kRequestHitPass = 1,
    kRequestHitFail = 2,
    kCacheCapacity = 3,
    kBoundaryViolation = 4,
    kBoundaryViolationInferred = 5,
    kRsbResolved = 6,
    kRsbFailed = 7,
    kOverrideApplied = 8,
};

inline constexpr std::size_t kLifecycleStateCount = 5;
inline constexpr std::size_t kLifecycleEventCount = 9;

struct TransitionEntry {
    LifecycleState from = LifecycleState::kActive;
    LifecycleEvent event = LifecycleEvent::kPlacementEvict;
    bool legal = false;
    LifecycleState to = LifecycleState::kActive;
};

[[nodiscard]] constexpr auto make_transition_entry(LifecycleState from,
                                                   LifecycleEvent event) noexcept
    -> TransitionEntry {
    if (from == LifecycleState::kActive && event == LifecycleEvent::kPlacementEvict) {
        return TransitionEntry{
            .from = from, .event = event, .legal = true, .to = LifecycleState::kCached};
    }
    if (from == LifecycleState::kCached && event == LifecycleEvent::kRequestHitPass) {
        return TransitionEntry{
            .from = from, .event = event, .legal = true, .to = LifecycleState::kActive};
    }
    if (from == LifecycleState::kCached && event == LifecycleEvent::kRequestHitFail) {
        return TransitionEntry{
            .from = from, .event = event, .legal = true, .to = LifecycleState::kStale};
    }
    if (from == LifecycleState::kCached && event == LifecycleEvent::kCacheCapacity) {
        return TransitionEntry{
            .from = from, .event = event, .legal = true, .to = LifecycleState::kStale};
    }
    if (from == LifecycleState::kActive && (event == LifecycleEvent::kBoundaryViolation ||
                                            event == LifecycleEvent::kBoundaryViolationInferred)) {
        return TransitionEntry{
            .from = from, .event = event, .legal = true, .to = LifecycleState::kQuarantined};
    }
    if (from == LifecycleState::kQuarantined && event == LifecycleEvent::kRsbResolved) {
        return TransitionEntry{
            .from = from, .event = event, .legal = true, .to = LifecycleState::kOverridden};
    }
    if (from == LifecycleState::kQuarantined && event == LifecycleEvent::kRsbFailed) {
        return TransitionEntry{
            .from = from, .event = event, .legal = true, .to = LifecycleState::kQuarantined};
    }
    if (from == LifecycleState::kOverridden && event == LifecycleEvent::kOverrideApplied) {
        return TransitionEntry{
            .from = from, .event = event, .legal = true, .to = LifecycleState::kActive};
    }
    return TransitionEntry{.from = from, .event = event, .legal = false, .to = from};
}

using LifecycleTable =
    std::array<std::array<TransitionEntry, kLifecycleEventCount>, kLifecycleStateCount>;

[[nodiscard]] constexpr auto build_lifecycle_table() noexcept -> LifecycleTable {
    LifecycleTable table{};
    for (std::size_t s = 0; s < kLifecycleStateCount; ++s) {
        for (std::size_t e = 0; e < kLifecycleEventCount; ++e) {
            table.at(s).at(e) = make_transition_entry(static_cast<LifecycleState>(s),
                                                      static_cast<LifecycleEvent>(e));
        }
    }
    return table;
}

inline constexpr LifecycleTable kLifecycleTransitionTable = build_lifecycle_table();

[[nodiscard]] constexpr auto validate_lifecycle_table_completeness() noexcept -> bool {
    std::size_t legal_count = 0;
    std::size_t illegal_count = 0;
    for (std::size_t s = 0; s < kLifecycleStateCount; ++s) {
        for (std::size_t e = 0; e < kLifecycleEventCount; ++e) {
            const TransitionEntry& entry = kLifecycleTransitionTable.at(s).at(e);
            if (entry.from != static_cast<LifecycleState>(s) ||
                entry.event != static_cast<LifecycleEvent>(e)) {
                return false;
            }
            if (entry.legal) {
                ++legal_count;
            } else {
                ++illegal_count;
            }
        }
    }
    return (legal_count == 9U) && (illegal_count == 36U);
}

static_assert(validate_lifecycle_table_completeness(),
              "Lifecycle transition table must cover all 45 (state, event) pairs with 9 legal and "
              "36 illegal entries");

[[nodiscard]] constexpr auto apply_runtime_transition(LifecycleState current,
                                                      LifecycleEvent event) noexcept
    -> core::Result<LifecycleState> {
    const auto s_idx = static_cast<std::size_t>(current);
    const auto e_idx = static_cast<std::size_t>(event);
    if (s_idx >= kLifecycleStateCount || e_idx >= kLifecycleEventCount) {
        return core::make_error(core::ErrorCode::kInvalidArgument,
                                "erg::apply_runtime_transition out-of-range state or event");
    }
    const TransitionEntry& entry = kLifecycleTransitionTable.at(s_idx).at(e_idx);
    if (!entry.legal) {
        return core::make_error(core::ErrorCode::kInvalidArgument,
                                "erg::apply_runtime_transition illegal lifecycle transition");
    }
    return entry.to;
}

// Typestate wrapper for compile-time enforced transitions.
// NOTE: Compile-time guarantees hold while a state is manipulated as TypedState<S>,
// and end at the homogeneous container boundary (e.g., std::vector<ErgNode>) where
// the tag S is erased into the runtime LifecycleState enum.
template <LifecycleState S> class TypedState {
public:
    explicit constexpr TypedState(rsb::StateId id) noexcept : id_(id) {}

    [[nodiscard]] constexpr auto state() const noexcept -> LifecycleState {
        return S;
    }

    [[nodiscard]] constexpr auto id() const noexcept -> rsb::StateId {
        return id_;
    }

    template <LifecycleEvent E>
        requires(kLifecycleTransitionTable.at(static_cast<std::size_t>(S))
                     .at(static_cast<std::size_t>(E))
                     .legal)
    [[nodiscard]] constexpr auto transition() const noexcept
        -> TypedState<kLifecycleTransitionTable.at(static_cast<std::size_t>(S))
                          .at(static_cast<std::size_t>(E))
                          .to> {
        constexpr LifecycleState kNext = kLifecycleTransitionTable.at(static_cast<std::size_t>(S))
                                             .at(static_cast<std::size_t>(E))
                                             .to;
        return TypedState<kNext>{id_};
    }

private:
    rsb::StateId id_{};
};

} // namespace hikari::erg
