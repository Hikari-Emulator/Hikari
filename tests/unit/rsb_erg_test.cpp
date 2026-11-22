#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <print>
#include <string_view>

#include "capture/fixture.hpp"
#include "capture/format.hpp"
#include "core/assert.hpp"
#include "core/hash.hpp"
#include "core/id.hpp"
#include "erg/erg.hpp"
#include "erg/hct_stub.hpp"
#include "rsb/rsb.hpp"
#include "validate/rsb_erg_replay.hpp"

namespace {

void test_rsb_determinism() {
    const std::array<hikari::rsb::ResourceFormat, 3> known_formats = {
        hikari::rsb::ResourceFormat::kRgba8Unorm,
        hikari::rsb::ResourceFormat::kR32Uint,
        hikari::rsb::ResourceFormat::kRgba16Float,
    };
    const auto allowed = hikari::erg::hct_allowed_formats(hikari::capture::GenerationId::kMaxwell);

    const hikari::rsb::Dependency dep{
        .consumer_op_index = 2U,
        .required_state_id = hikari::rsb::StateId{.index = 2U, .generation = 1U},
        .required_resource_id = hikari::core::ResourceId{.index = 3U, .generation = 1U},
    };
    const hikari::rsb::ResolveContext ctx{
        .seed = 0x5EED1234ULL,
        .next_spec_id = 7U,
        .assumed_resource_version = 0U,
        .default_dimensions = hikari::rsb::Dimensions{.width = 64U, .height = 64U, .depth = 1U},
        .default_binding_flags = 1U,
        .capture_known_formats = known_formats,
        .hct_allowed_formats = allowed,
    };

    std::array<hikari::rsb::Candidate, 3> candidates{};
    for (std::size_t run = 0; run < candidates.size(); ++run) {
        const auto res = hikari::rsb::Rsb::resolve(dep, ctx);
        HIKARI_CHECK(res.has_value());
        candidates.at(run) = *res;
        const auto hex = hikari::core::format_hash128(res->state.content_hash);
        std::println("rsb_determinism run={} spec_id={} format={} guard_res={}:{} guard_ver={} "
                     "state_hash={}",
                     run, res->spec_id, static_cast<unsigned>(res->state.format),
                     res->guard.resource_id.index, res->guard.resource_id.generation,
                     res->guard.expected_version, std::string_view{hex.data(), 32});
    }

    HIKARI_CHECK(candidates.at(0) == candidates.at(1));
    HIKARI_CHECK(candidates.at(0) == candidates.at(2));
}

void test_rsb_bound() {
    const auto allowed = hikari::erg::hct_allowed_formats(hikari::capture::GenerationId::kMaxwell);

    // Every candidate produced from known formats must belong to the HCT allowed set.
    const std::array<hikari::rsb::ResourceFormat, 3> mixed_formats = {
        hikari::rsb::ResourceFormat::kInvalidExperimentalBc7,
        hikari::rsb::ResourceFormat::kRgba8Unorm,
        hikari::rsb::ResourceFormat::kR32Uint,
    };
    const hikari::rsb::Dependency dep{
        .consumer_op_index = 2U,
        .required_state_id = hikari::rsb::StateId{.index = 2U, .generation = 1U},
        .required_resource_id = hikari::core::ResourceId{.index = 3U, .generation = 1U},
    };
    for (std::uint64_t seed = 0; seed < 16U; ++seed) {
        const hikari::rsb::ResolveContext ctx{
            .seed = seed,
            .next_spec_id = seed + 1U,
            .assumed_resource_version = 0U,
            .capture_known_formats = mixed_formats,
            .hct_allowed_formats = allowed,
        };
        const auto res = hikari::rsb::Rsb::resolve(dep, ctx);
        HIKARI_CHECK(res.has_value());
        HIKARI_CHECK(hikari::erg::hct_is_format_allowed(hikari::capture::GenerationId::kMaxwell,
                                                        res->state.format));
    }

    // Candidate set containing ONLY a format outside the HCT allowed set must be rejected.
    const std::array<hikari::rsb::ResourceFormat, 1> invalid_only = {
        hikari::rsb::ResourceFormat::kInvalidExperimentalBc7,
    };
    const hikari::rsb::ResolveContext bad_ctx{
        .seed = 0x5EED1234ULL,
        .next_spec_id = 1U,
        .assumed_resource_version = 0U,
        .capture_known_formats = invalid_only,
        .hct_allowed_formats = allowed,
    };
    const auto rejected = hikari::rsb::Rsb::resolve(dep, bad_ctx);
    HIKARI_CHECK(!rejected.has_value());
    std::println("rsb_bound: invalid candidate format 999 rejected with code={}",
                 static_cast<unsigned>(rejected.error().code));
}

void test_erg_liveness_on_fixture_e() {
    const auto fix_e = hikari::capture::build_dead_write_fixture();
    HIKARI_CHECK(fix_e.has_value());

    const auto outcome = hikari::validate::replay_rsb_erg_hashes(*fix_e);
    HIKARI_CHECK(outcome.has_value());
    HIKARI_CHECK(outcome->frames.size() == 3U);

    // Frame 0 has 3 ops (dead ClearOp, overwriting ClearOp, DrawPatternOp):
    // exactly 1 node (the dead ClearOp) must be removed, leaving 2 nodes.
    HIKARI_CHECK(outcome->frames.at(0).nodes_before_liveness == 3U);
    HIKARI_CHECK(outcome->frames.at(0).nodes_removed_by_liveness == 1U);
    HIKARI_CHECK(outcome->frames.at(0).nodes_after_liveness == 2U);

    // Frames 1 and 2 must have 0 nodes removed.
    HIKARI_CHECK(outcome->frames.at(1).nodes_removed_by_liveness == 0U);
    HIKARI_CHECK(outcome->frames.at(2).nodes_removed_by_liveness == 0U);
    std::println("erg_liveness fixture=E frame0_before={} frame0_removed={} frame0_after={} "
                 "frame1_removed={} frame2_removed={}",
                 outcome->frames.at(0).nodes_before_liveness,
                 outcome->frames.at(0).nodes_removed_by_liveness,
                 outcome->frames.at(0).nodes_after_liveness,
                 outcome->frames.at(1).nodes_removed_by_liveness,
                 outcome->frames.at(2).nodes_removed_by_liveness);
}

void test_erg_relevance_never_removes() {
    hikari::erg::Erg graph;
    const std::array<std::int64_t, 5> extreme_hotness = {
        std::numeric_limits<std::int64_t>::min(), -1000000, 0, 1000000,
        std::numeric_limits<std::int64_t>::max(),
    };

    for (std::size_t i = 0; i < extreme_hotness.size(); ++i) {
        const auto idx = static_cast<std::uint32_t>(i + 1U);
        graph.add_node(hikari::erg::ErgNode{
            .state =
                hikari::rsb::SemanticState{
                    .state_id = hikari::rsb::StateId{.index = idx, .generation = 1U},
                    .resource_id = hikari::core::ResourceId{.index = idx, .generation = 1U},
                    .format = hikari::rsb::ResourceFormat::kRgba8Unorm,
                    .producer_op_index = idx,
                },
            .hotness = extreme_hotness.at(i),
        });
    }

    const std::size_t count_before = graph.nodes().size();
    const auto order = graph.compute_relevance_ordering();
    const std::size_t count_after = graph.nodes().size();

    HIKARI_CHECK(count_before == 5U);
    HIKARI_CHECK(count_after == count_before);
    HIKARI_CHECK(order.size() == count_before);
    // Highest hotness (index 4) must be ordered first; lowest hotness (index 0) last.
    HIKARI_CHECK(order.front() == 4U);
    HIKARI_CHECK(order.back() == 0U);
    std::println("erg_relevance_never_removes: count_before={} count_after={} ordered_size={}",
                 count_before, count_after, order.size());
}

} // namespace

int main() {
    try {
        test_rsb_determinism();
        test_rsb_bound();
        test_erg_liveness_on_fixture_e();
        test_erg_relevance_never_removes();
        std::println("rsb_erg_test: all checks passed");
        return 0;
    } catch (...) {
        return 1;
    }
}
