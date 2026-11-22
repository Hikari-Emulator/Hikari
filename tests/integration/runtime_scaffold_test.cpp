#include <array>
#include <cstddef>
#include <cstdint>
#include <print>
#include <string_view>

#include "capture/fixture.hpp"
#include "core/assert.hpp"
#include "core/hash.hpp"
#include "core/result.hpp"
#include "cpu/cpu_backend.hpp"
#include "exec/gfx/executor.hpp"
#include "gpu/gen/generations.hpp"
#include "loader/loader.hpp"
#include "runtime/frame_loop.hpp"
#include "services/service_host.hpp"

namespace {

void test_stubs_return_not_implemented() {
    hikari::cpu::DynarmicBackend dynarmic;
    hikari::cpu::InterpreterBackend interpreter;
    const std::array<std::uint32_t, 1> words = {0U};
    HIKARI_CHECK(!dynarmic.step_frame(words).has_value());
    HIKARI_CHECK(dynarmic.step_frame(words).error().code ==
                 hikari::core::ErrorCode::kNotImplemented);
    HIKARI_CHECK(!interpreter.step_frame(words).has_value());
    HIKARI_CHECK(interpreter.step_frame(words).error().code ==
                 hikari::core::ErrorCode::kNotImplemented);

    hikari::gpu::gen::CaptureSink sink{};
    const std::uint32_t cmd = 1U;
    HIKARI_CHECK(hikari::gpu::gen::MaxwellGen::decode(&cmd, sink).has_value());
    const auto amp = hikari::gpu::gen::AmpereGen::decode(&cmd, sink);
    HIKARI_CHECK(!amp.has_value());
    HIKARI_CHECK(amp.error().code == hikari::core::ErrorCode::kNotImplemented);

    hikari::services::MinimalServiceHost svc;
    HIKARI_CHECK(svc.query_clock_ticks().has_value());
    HIKARI_CHECK(svc.open_filesystem("romfs").error().code ==
                 hikari::core::ErrorCode::kNotImplemented);
    HIKARI_CHECK(svc.poll_input().error().code == hikari::core::ErrorCode::kNotImplemented);
    HIKARI_CHECK(svc.submit_audio().error().code == hikari::core::ErrorCode::kNotImplemented);

    const std::array<std::byte, 4> img_bytes = {std::byte{0x01}, std::byte{0x02}, std::byte{0x03},
                                                std::byte{0x04}};
    HIKARI_CHECK(hikari::loader::Loader::parse_synthetic_image(img_bytes).has_value());
    HIKARI_CHECK(hikari::loader::Loader::load_commercial_nro(img_bytes).error().code ==
                 hikari::core::ErrorCode::kNotImplemented);

    const hikari::gfx::CommandBuffer cmd_buf{1U};
    HIKARI_CHECK(hikari::gfx::submit_to_vulkan_device(cmd_buf).error().code ==
                 hikari::core::ErrorCode::kNotImplemented);
}

void test_guest_memory_dirty_ranges() {
    hikari::loader::GuestMemory mem{4096U};
    const std::uint64_t e0 = mem.current_epoch();
    const std::array<std::byte, 8> payload = {
        std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4},
        std::byte{5}, std::byte{6}, std::byte{7}, std::byte{8},
    };
    const auto e1 = mem.write(256U, payload);
    HIKARI_CHECK(e1.has_value());
    HIKARI_CHECK(mem.is_range_dirty_after(256U, 264U, e0));
    HIKARI_CHECK(!mem.is_range_dirty_after(256U, 264U, *e1));
    HIKARI_CHECK(!mem.is_range_dirty_after(512U, 520U, e0));
}

void test_single_vs_threaded_comparator() {
    const auto fixture = hikari::capture::build_synthetic_fixture();
    HIKARI_CHECK(fixture.has_value());

    const auto single_rep = hikari::runtime::FrameLoop::run_capture(
        *fixture, hikari::runtime::RuntimeConfig{
                      .mode = hikari::runtime::ExecutionMode::kSingleThreaded,
                      .seed = 0x5EED1234ULL,
                  });
    const auto threaded_rep = hikari::runtime::FrameLoop::run_capture(
        *fixture, hikari::runtime::RuntimeConfig{
                      .mode = hikari::runtime::ExecutionMode::kThreadedSpsc,
                      .seed = 0x5EED1234ULL,
                  });

    HIKARI_CHECK(single_rep.has_value());
    HIKARI_CHECK(threaded_rep.has_value());
    HIKARI_CHECK(single_rep->comparisons.size() == 3U);
    HIKARI_CHECK(threaded_rep->comparisons.size() == 3U);

    for (std::size_t i = 0; i < 3U; ++i) {
        const auto& s = single_rep->comparisons.at(i);
        const auto& t = threaded_rep->comparisons.at(i);
        HIKARI_CHECK(s.main_hash == t.main_hash);
        HIKARI_CHECK(s.reference_hash == t.reference_hash);
        HIKARI_CHECK(s.matched && t.matched);
        const auto hex = hikari::core::format_hash128(s.main_hash);
        std::println("runtime_comparator frame={} single={} threaded={} status=MATCH",
                     s.frame_index.value, std::string_view{hex.data(), 32},
                     std::string_view{hex.data(), 32});
    }
}

} // namespace

int main() {
    try {
        test_stubs_return_not_implemented();
        test_guest_memory_dirty_ranges();
        test_single_vs_threaded_comparator();
        std::println("runtime_scaffold_test: all checks passed");
        return 0;
    } catch (...) {
        return 1;
    }
}
