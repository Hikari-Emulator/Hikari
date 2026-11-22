#include <print>
#include <span>
#include <string_view>

#include "capture/fixture.hpp"
#include "capture/reader.hpp"
#include "core/assert.hpp"
#include "core/hash.hpp"
#include "exec/specialized_selector.hpp"
#include "gpu/frontend/frontend.hpp"

namespace {

void compare_specialized_vs_generic(std::string_view label,
                                    std::span<const std::byte> capture_bytes) {
    const auto reader = hikari::capture::CaptureReader::open(capture_bytes);
    HIKARI_CHECK(reader.has_value());

    hikari::gpu::frontend::StateTracker tracker;
    const auto packets =
        hikari::gpu::frontend::emit_ir_packets_from_capture(*reader, 0x5EED1234ULL, tracker);
    HIKARI_CHECK(packets.has_value());

    for (const auto& pkt : *packets) {
        const auto sig = hikari::exec::classify_packet_signature(pkt);
        const auto spec_hash = hikari::exec::execute_packet_with_specialized_selection(pkt);
        const auto gen_hash = hikari::exec::execute_packet_generic(pkt);
        HIKARI_CHECK(spec_hash == gen_hash);
        const auto hex = hikari::core::format_hash128(spec_hash);
        std::println("specialized_comparator fixture={} frame={} signature={} specialized={} "
                     "generic={} status=MATCH",
                     label, pkt.frame_index.value, static_cast<unsigned>(sig),
                     std::string_view{hex.data(), 32}, std::string_view{hex.data(), 32});
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

        compare_specialized_vs_generic("A_synthetic", *fix_a);
        compare_specialized_vs_generic("B_barrier_reordered", *fix_b);
        compare_specialized_vs_generic("C_empty_frame", *fix_c);
        compare_specialized_vs_generic("D_incomplete_dep", *fix_d);
        compare_specialized_vs_generic("E_dead_write", *fix_e);

        std::println("specialized_selector_test: all checks passed");
        return 0;
    } catch (...) {
        return 1;
    }
}
