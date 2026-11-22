#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "core/result.hpp"

namespace hikari::loader {

struct DirtyRange {
    std::uint64_t guest_range_start = 0;
    std::uint64_t guest_range_end = 0;
    std::uint64_t epoch = 0;
};

class GuestMemory {
public:
    explicit GuestMemory(std::size_t size_bytes = 65536U);

    [[nodiscard]] auto write(std::uint64_t guest_addr, std::span<const std::byte> data)
        -> core::Result<std::uint64_t>;
    [[nodiscard]] auto read(std::uint64_t guest_addr, std::size_t len) const noexcept
        -> core::Result<std::span<const std::byte>>;

    [[nodiscard]] auto is_range_dirty_after(std::uint64_t start, std::uint64_t end,
                                            std::uint64_t epoch) const noexcept -> bool;
    [[nodiscard]] auto current_epoch() const noexcept -> std::uint64_t {
        return epoch_;
    }

private:
    std::vector<std::byte> storage_;
    std::vector<DirtyRange> dirty_log_;
    std::uint64_t epoch_ = 0;
};

struct SyntheticImage {
    std::uint64_t entry_point = 0x80000000ULL;
    std::vector<std::byte> payload;
};

class Loader {
public:
    [[nodiscard]] static auto parse_synthetic_image(std::span<const std::byte> bytes)
        -> core::Result<SyntheticImage>;
    [[nodiscard]] static auto load_commercial_nro(std::span<const std::byte> bytes) noexcept
        -> core::Result<SyntheticImage>;
};

} // namespace hikari::loader
