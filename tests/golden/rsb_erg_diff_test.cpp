#include <array>
#include <cstddef>
#include <print>
#include <span>
#include <string_view>
#include <vector>

#include "capture/fixture.hpp"
#include "capture/format.hpp"
#include "capture/reader.hpp"
#include "capture/writer.hpp"
#include "core/assert.hpp"
#include "core/hash.hpp"
#include "core/id.hpp"
#include "core/result.hpp"
#include "rsb/rsb.hpp"
#include "validate/reference_replay.hpp"
#include "validate/rsb_erg_replay.hpp"

namespace {

auto compare_fixture(std::string_view name, std::span<const std::byte> capture_bytes)
    -> std::size_t {
    const auto reader = hikari::capture::CaptureReader::open(capture_bytes);
    HIKARI_CHECK(reader.has_value());

    std::vector<hikari::validate::FrameSemanticHash> ref_hashes(reader->header().frame_count);
    const auto ref_res = hikari::validate::replay_semantic_hashes(*reader, ref_hashes);
    const auto rsb_res = hikari::validate::replay_rsb_erg_hashes(*reader);

    if (!ref_res.has_value()) {
        HIKARI_CHECK(rsb_res.has_value());
        for (const auto& f : rsb_res->frames) {
            const auto rsb_hex = hikari::core::format_hash128(f.frame_hash.semantic_content_hash);
            std::println(
                "diff fixture={} frame={} reference=ERROR(code={}) rsb_erg={} "
                "status=reference error, comparison not possible spec_candidates={} pruned={}",
                name, f.frame_hash.frame_index.value, static_cast<unsigned>(ref_res.error().code),
                std::string_view{rsb_hex.data(), 32}, f.speculative_candidates_resolved,
                f.nodes_removed_by_liveness);
        }
        return rsb_res->frames.size();
    }

    HIKARI_CHECK(rsb_res.has_value());
    HIKARI_CHECK(*ref_res == rsb_res->frames.size());

    std::size_t mismatches = 0;
    for (std::size_t i = 0; i < *ref_res; ++i) {
        const auto& ref_f = ref_hashes.at(i);
        const auto& rsb_f = rsb_res->frames.at(i);
        const bool match = ref_f.semantic_content_hash == rsb_f.frame_hash.semantic_content_hash;
        if (!match) {
            ++mismatches;
        }
        const auto ref_hex = hikari::core::format_hash128(ref_f.semantic_content_hash);
        const auto rsb_hex = hikari::core::format_hash128(rsb_f.frame_hash.semantic_content_hash);
        std::println("diff fixture={} frame={} reference={} rsb_erg={} status={} "
                     "spec_candidates={} pruned={}",
                     name, ref_f.frame_index.value, std::string_view{ref_hex.data(), 32},
                     std::string_view{rsb_hex.data(), 32}, match ? "MATCH" : "MISMATCH",
                     rsb_f.speculative_candidates_resolved, rsb_f.nodes_removed_by_liveness);
    }
    return mismatches;
}

void verify_fixture_d_predictions(std::span<const std::byte> fix_d) {
    // 1. Reference path must report an error (kInvalidArgument) on unresolved resource {3, 1}.
    std::array<hikari::validate::FrameSemanticHash, 1> ref_out{};
    const auto ref_d = hikari::validate::replay_semantic_hashes(fix_d, ref_out);
    HIKARI_CHECK(!ref_d.has_value());
    HIKARI_CHECK(ref_d.error().code == hikari::core::ErrorCode::kInvalidArgument);

    // 2. RSB+ERG path must resolve 1 speculative candidate for {3, 1} with kRgba8Unorm.
    const auto rsb_d = hikari::validate::replay_rsb_erg_hashes(fix_d);
    HIKARI_CHECK(rsb_d.has_value());
    HIKARI_CHECK(rsb_d->frames.size() == 1U);
    const auto& frame0 = rsb_d->frames.at(0);
    HIKARI_CHECK(frame0.speculative_candidates_resolved == 1U);
    HIKARI_CHECK(frame0.candidates.size() == 1U);
    const auto& cand = frame0.candidates.at(0);
    HIKARI_CHECK(cand.speculative);
    HIKARI_CHECK(cand.spec_id == 1U);
    HIKARI_CHECK(cand.state.resource_id ==
                 (hikari::core::ResourceId{.index = 3U, .generation = 1U}));
    HIKARI_CHECK(cand.state.format == hikari::rsb::ResourceFormat::kRgba8Unorm);
    HIKARI_CHECK(cand.guard.resource_id ==
                 (hikari::core::ResourceId{.index = 3U, .generation = 1U}));
    HIKARI_CHECK(cand.guard.expected_version == 0U);
}

void verify_fixture_e_predictions(std::span<const std::byte> fix_a,
                                  std::span<const std::byte> fix_e) {
    std::array<hikari::validate::FrameSemanticHash, 3> ref_a{};
    const auto count_a = hikari::validate::replay_semantic_hashes(fix_a, ref_a);
    HIKARI_CHECK(count_a.has_value() && *count_a == 3U);

    const auto rsb_e = hikari::validate::replay_rsb_erg_hashes(fix_e);
    HIKARI_CHECK(rsb_e.has_value() && rsb_e->frames.size() == 3U);

    // After ERG liveness removes the extra dead write in Fixture E Frame 0,
    // RSB+ERG on Fixture E must match Reference on Fixture A for every frame (0, 1, 2).
    for (std::size_t i = 0; i < 3U; ++i) {
        HIKARI_CHECK(rsb_e->frames.at(i).frame_hash.semantic_content_hash ==
                     ref_a.at(i).semantic_content_hash);
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
        HIKARI_CHECK(fix_a.has_value());
        HIKARI_CHECK(fix_b.has_value());
        HIKARI_CHECK(fix_c.has_value());
        HIKARI_CHECK(fix_d.has_value());
        HIKARI_CHECK(fix_e.has_value());

        const std::size_t mis_a = compare_fixture("A_synthetic", *fix_a);
        const std::size_t mis_b = compare_fixture("B_barrier_reordered", *fix_b);
        const std::size_t mis_c = compare_fixture("C_empty_frame", *fix_c);
        const std::size_t mis_d = compare_fixture("D_incomplete_dep", *fix_d);
        const std::size_t mis_e = compare_fixture("E_dead_write", *fix_e);

        HIKARI_CHECK(mis_a == 0U);
        HIKARI_CHECK(mis_b == 0U);
        HIKARI_CHECK(mis_c == 0U);
        HIKARI_CHECK(mis_d == 1U);
        // On Fixture E directly, unpruned Reference(E) includes the dead write in Frame 0 while
        // pruned RSB+ERG(E) removes it (so Frame 0 differs vs Reference(E) and matches
        // Reference(A)).
        HIKARI_CHECK(mis_e == 1U);

        verify_fixture_d_predictions(*fix_d);
        verify_fixture_e_predictions(*fix_a, *fix_e);

        std::println("rsb_erg_diff_test: all differential checks completed");
        return 0;
    } catch (...) {
        return 1;
    }
}
