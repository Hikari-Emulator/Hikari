#include <cstddef>
#include <print>

#include "core/assert.hpp"
#include "erg/lifecycle.hpp"
#include "rsb/rsb.hpp"

int main() {
    try {
        // 1. Compile-time typestate transition chain.
        constexpr hikari::rsb::StateId kId{.index = 1U, .generation = 1U};
        constexpr hikari::erg::TypedState<hikari::erg::LifecycleState::kActive> kActive{kId};
        constexpr auto kCached = kActive.transition<hikari::erg::LifecycleEvent::kPlacementEvict>();
        static_assert(kCached.state() == hikari::erg::LifecycleState::kCached);
        constexpr auto kReactivated =
            kCached.transition<hikari::erg::LifecycleEvent::kRequestHitPass>();
        static_assert(kReactivated.state() == hikari::erg::LifecycleState::kActive);
        constexpr auto kQuarantined =
            kReactivated.transition<hikari::erg::LifecycleEvent::kBoundaryViolation>();
        static_assert(kQuarantined.state() == hikari::erg::LifecycleState::kQuarantined);
        constexpr auto kOverridden =
            kQuarantined.transition<hikari::erg::LifecycleEvent::kRsbResolved>();
        static_assert(kOverridden.state() == hikari::erg::LifecycleState::kOverridden);
        constexpr auto kFinalActive =
            kOverridden.transition<hikari::erg::LifecycleEvent::kOverrideApplied>();
        static_assert(kFinalActive.state() == hikari::erg::LifecycleState::kActive);

        // 2. Runtime comparator: drive every (state, event) pair and verify the table's answer.
        std::size_t legal_count = 0;
        std::size_t illegal_count = 0;
        for (std::size_t s = 0; s < hikari::erg::kLifecycleStateCount; ++s) {
            for (std::size_t e = 0; e < hikari::erg::kLifecycleEventCount; ++e) {
                const auto state = static_cast<hikari::erg::LifecycleState>(s);
                const auto event = static_cast<hikari::erg::LifecycleEvent>(e);
                const auto& entry = hikari::erg::kLifecycleTransitionTable.at(s).at(e);
                const auto res = hikari::erg::apply_runtime_transition(state, event);
                if (entry.legal) {
                    HIKARI_CHECK(res.has_value());
                    HIKARI_CHECK(*res == entry.to);
                    ++legal_count;
                } else {
                    HIKARI_CHECK(!res.has_value());
                    ++illegal_count;
                }
            }
        }

        std::println("lifecycle_typestate_test: total_pairs={} legal={} illegal={} status=MATCH",
                     legal_count + illegal_count, legal_count, illegal_count);
        HIKARI_CHECK(legal_count == 9U);
        HIKARI_CHECK(illegal_count == 36U);
        return 0;
    } catch (...) {
        return 1;
    }
}
