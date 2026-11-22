#include <array>
#include <cstddef>
#include <print>
#include <string_view>

#include "capture/fixture.hpp"
#include "core/assert.hpp"
#include "core/hash.hpp"
#include "validate/reference_replay.hpp"

namespace {

constexpr std::size_t kFramesPerFixture = 3;
using FixtureHashes = std::array<hikari::validate::FrameSemanticHash, kFramesPerFixture>;

void print_fixture_hashes(std::string_view label, const FixtureHashes& hashes) {
    for (const auto& entry : hashes) {
        const auto hex = hikari::core::format_hash128(entry.semantic_content_hash);
        std::println("fixture={} frame={} semantic_content_hash={}", label, entry.frame_index.value,
                     std::string_view{hex.data(), 32});
    }
}

} // namespace

int main() {
    try {
        const auto fix_a = hikari::capture::build_synthetic_fixture();
        const auto fix_b = hikari::capture::build_barrier_reordered_fixture();
        const auto fix_c = hikari::capture::build_empty_frame_fixture();
        HIKARI_CHECK(fix_a.has_value());
        HIKARI_CHECK(fix_b.has_value());
        HIKARI_CHECK(fix_c.has_value());

        FixtureHashes hashes_a{};
        FixtureHashes hashes_b{};
        FixtureHashes hashes_c{};

        const auto count_a = hikari::validate::replay_semantic_hashes(*fix_a, hashes_a);
        const auto count_b = hikari::validate::replay_semantic_hashes(*fix_b, hashes_b);
        const auto count_c = hikari::validate::replay_semantic_hashes(*fix_c, hashes_c);
        HIKARI_CHECK(count_a.has_value() && *count_a == kFramesPerFixture);
        HIKARI_CHECK(count_b.has_value() && *count_b == kFramesPerFixture);
        HIKARI_CHECK(count_c.has_value() && *count_c == kFramesPerFixture);

        print_fixture_hashes("A_synthetic", hashes_a);
        print_fixture_hashes("B_barrier_reordered", hashes_b);
        print_fixture_hashes("C_empty_frame", hashes_c);

        // Fixture B relations vs Fixture A:
        // Frame 0 and Frame 1 must match; Frame 2 (reordered barrier/ops) must differ.
        HIKARI_CHECK(hashes_b.at(0).semantic_content_hash == hashes_a.at(0).semantic_content_hash);
        HIKARI_CHECK(hashes_b.at(1).semantic_content_hash == hashes_a.at(1).semantic_content_hash);
        HIKARI_CHECK(hashes_b.at(2).semantic_content_hash != hashes_a.at(2).semantic_content_hash);

        // Fixture C relations vs Fixture A:
        // Frame 0 and Frame 2 must match; Frame 1 (empty frame) must differ.
        HIKARI_CHECK(hashes_c.at(0).semantic_content_hash == hashes_a.at(0).semantic_content_hash);
        HIKARI_CHECK(hashes_c.at(1).semantic_content_hash != hashes_a.at(1).semantic_content_hash);
        HIKARI_CHECK(hashes_c.at(2).semantic_content_hash == hashes_a.at(2).semantic_content_hash);

        std::println("fixture_relations_test: all predicted relations satisfied");
        return 0;
    } catch (...) {
        return 1;
    }
}
