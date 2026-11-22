#include <print>
#include <string_view>

#include "capture/fixture.hpp"
#include "capture/reader.hpp"
#include "checkpoint/speculative_override.hpp"
#include "core/assert.hpp"
#include "core/hash.hpp"
#include "core/id.hpp"

int main() {
    try {
        const auto fix_a = hikari::capture::build_synthetic_fixture();
        HIKARI_CHECK(fix_a.has_value());
        const auto reader = hikari::capture::CaptureReader::open(*fix_a);
        HIKARI_CHECK(reader.has_value());

        // Quarantine on frame 0; override applied at frame 1 safe point.
        const auto report = hikari::checkpoint::run_speculative_override_with_comparator(
            *reader, hikari::core::FrameIndex{.value = 0U});
        HIKARI_CHECK(report.has_value());
        HIKARI_CHECK(report->frames.size() == 3U);
        HIKARI_CHECK(report->override_log.size() == 1U);

        // Frame 0 is SPECULATIVE and differs from reference; Frame 1 applies override and
        // converges.
        HIKARI_CHECK(report->frames.at(0).differs_from_reference);
        HIKARI_CHECK(!report->frames.at(1).differs_from_reference);
        HIKARI_CHECK(!report->frames.at(2).differs_from_reference);
        HIKARI_CHECK(report->frames_until_convergence == 1U);

        for (const auto& f : report->frames) {
            const auto spec_hex = hikari::core::format_hash128(f.speculative_hash);
            const auto ref_hex = hikari::core::format_hash128(f.reference_hash);
            std::println("spec_override frame={} status={} speculative={} reference={} differs={}",
                         f.frame_index.value, static_cast<unsigned>(f.status),
                         std::string_view{spec_hex.data(), 32},
                         std::string_view{ref_hex.data(), 32},
                         f.differs_from_reference ? "true" : "false");
        }
        std::println("spec_override_summary quarantine_frame={} override_frame={} "
                     "converged_frame={} frames_until_convergence={}",
                     report->quarantine_frame.value, report->override_applied_frame.value,
                     report->converged_frame.value, report->frames_until_convergence);
        return 0;
    } catch (...) {
        return 1;
    }
}
