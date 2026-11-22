#include <array>
#include <cstddef>
#include <print>
#include <string_view>

#include "capture/fixture.hpp"
#include "core/assert.hpp"
#include "core/hash.hpp"
#include "validate/reference_replay.hpp"

int main() {
    try {
        const auto fixture = hikari::capture::build_synthetic_fixture();
        HIKARI_CHECK(fixture.has_value());

        constexpr std::size_t kExpectedFrames = 3;
        std::array<std::array<hikari::validate::FrameSemanticHash, kExpectedFrames>, 3> runs{};

        for (std::size_t run_idx = 0; run_idx < runs.size(); ++run_idx) {
            const auto count = hikari::validate::replay_semantic_hashes(*fixture, runs.at(run_idx));
            HIKARI_CHECK(count.has_value());
            HIKARI_CHECK(*count == kExpectedFrames);

            for (std::size_t f = 0; f < kExpectedFrames; ++f) {
                const auto& entry = runs.at(run_idx).at(f);
                const auto hex = hikari::core::format_hash128(entry.semantic_content_hash);
                std::println("run={} frame={} semantic_content_hash={}", run_idx,
                             entry.frame_index.value, std::string_view{hex.data(), 32});
            }
        }

        for (std::size_t f = 0; f < kExpectedFrames; ++f) {
            HIKARI_CHECK(runs.at(0).at(f).frame_index == runs.at(1).at(f).frame_index);
            HIKARI_CHECK(runs.at(0).at(f).frame_index == runs.at(2).at(f).frame_index);
            HIKARI_CHECK(runs.at(0).at(f).semantic_content_hash ==
                         runs.at(1).at(f).semantic_content_hash);
            HIKARI_CHECK(runs.at(0).at(f).semantic_content_hash ==
                         runs.at(2).at(f).semantic_content_hash);
        }

        std::println("determinism_test: 3 runs produced identical semantic content hashes");
        return 0;
    } catch (...) {
        return 1;
    }
}
