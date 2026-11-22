#include <cstddef>
#include <print>
#include <span>
#include <string_view>

#include "capture/fixture.hpp"
#include "capture/reader.hpp"
#include "core/assert.hpp"
#include "core/hash.hpp"
#include "erg/placement_policy.hpp"

namespace {

void check_placement_policy_comparator(std::string_view label,
                                       std::span<const std::byte> capture_bytes) {
    const auto reader = hikari::capture::CaptureReader::open(capture_bytes);
    HIKARI_CHECK(reader.has_value());

    const hikari::erg::AlwaysActivePlacementPolicy policy_off;
    const hikari::erg::ThresholdPlacementPolicy policy_on{15};

    const auto off_res = hikari::erg::replay_with_placement_policy(*reader, policy_off);
    const auto on_res = hikari::erg::replay_with_placement_policy(*reader, policy_on);
    HIKARI_CHECK(off_res.has_value());
    HIKARI_CHECK(on_res.has_value());
    HIKARI_CHECK(off_res->size() == on_res->size());

    for (std::size_t i = 0; i < off_res->size(); ++i) {
        const auto& off_f = off_res->at(i);
        const auto& on_f = on_res->at(i);
        HIKARI_CHECK(off_f.frame_hash.semantic_content_hash ==
                     on_f.frame_hash.semantic_content_hash);
        const auto hex = hikari::core::format_hash128(on_f.frame_hash.semantic_content_hash);
        std::println("h7_placement fixture={} frame={} policy_off={} policy_on={} cached={} "
                     "reactivated={} status=MATCH",
                     label, on_f.frame_hash.frame_index.value, std::string_view{hex.data(), 32},
                     std::string_view{hex.data(), 32}, on_f.cached_count, on_f.reactivated_count);
    }
}

} // namespace

int main() {
    try {
        const auto fix_a = hikari::capture::build_synthetic_fixture();
        const auto fix_b = hikari::capture::build_barrier_reordered_fixture();
        const auto fix_c = hikari::capture::build_empty_frame_fixture();
        const auto fix_d = hikari::capture::build_incomplete_dependency_fixture();
        const auto fix_e = hikari::capture::build_dead_write_fixture();
        HIKARI_CHECK(fix_a.has_value() && fix_b.has_value() && fix_c.has_value() &&
                     fix_d.has_value() && fix_e.has_value());

        check_placement_policy_comparator("A_synthetic", *fix_a);
        check_placement_policy_comparator("B_barrier_reordered", *fix_b);
        check_placement_policy_comparator("C_empty_frame", *fix_c);
        check_placement_policy_comparator("D_incomplete_dep", *fix_d);
        check_placement_policy_comparator("E_dead_write", *fix_e);

        std::println("h7_relevance_placement_diff_test: all checks passed");
        return 0;
    } catch (...) {
        return 1;
    }
}
