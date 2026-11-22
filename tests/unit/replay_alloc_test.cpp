#include <array>
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <new>
#include <print>

#include "capture/fixture.hpp"
#include "capture/reader.hpp"
#include "core/assert.hpp"
#include "validate/reference_replay.hpp"

namespace {

auto count_allocations_flag() noexcept -> std::atomic<bool>& {
    static std::atomic<bool> flag{false};
    return flag;
}

auto allocation_counter() noexcept -> std::atomic<std::size_t>& {
    static std::atomic<std::size_t> count{0};
    return count;
}

} // namespace

void* operator new(std::size_t size) {
    if (count_allocations_flag().load(std::memory_order_relaxed)) {
        allocation_counter().fetch_add(1U, std::memory_order_relaxed);
    }
    const std::size_t bytes = size == 0U ? 1U : size;
    // clang-format off
    void* ptr = std::malloc(bytes); // NOLINT(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory): global operator new delegates to malloc
    // clang-format on
    if (ptr == nullptr) {
        throw std::bad_alloc{};
    }
    return ptr;
}

void operator delete(void* ptr) noexcept {
    // clang-format off
    std::free(ptr); // NOLINT(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory): global operator delete delegates to free
    // clang-format on
}

void operator delete(void* ptr, std::size_t /*size*/) noexcept {
    // clang-format off
    std::free(ptr); // NOLINT(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory): global operator delete delegates to free
    // clang-format on
}

int main() {
    try {
        // Setup allocations (fixture, CaptureReader, output span buffer) happen before arming the
        // counter.
        const auto fixture = hikari::capture::build_synthetic_fixture();
        HIKARI_CHECK(fixture.has_value());

        const auto reader = hikari::capture::CaptureReader::open(*fixture);
        HIKARI_CHECK(reader.has_value());

        std::array<hikari::validate::FrameSemanticHash, 3> out_frames{};

        allocation_counter().store(0U, std::memory_order_seq_cst);
        count_allocations_flag().store(true, std::memory_order_seq_cst);

        const auto count = hikari::validate::replay_semantic_hashes(*reader, out_frames);

        count_allocations_flag().store(false, std::memory_order_seq_cst);
        const std::size_t allocs = allocation_counter().load(std::memory_order_seq_cst);

        std::println("replay_alloc_test: counted_allocations={}", allocs);
        HIKARI_CHECK(count.has_value());
        HIKARI_CHECK(*count == 3U);
        HIKARI_CHECK(allocs == 0U);
        return 0;
    } catch (...) {
        return 1;
    }
}
