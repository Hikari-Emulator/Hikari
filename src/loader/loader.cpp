#include "loader/loader.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "core/result.hpp"

namespace hikari::loader {

GuestMemory::GuestMemory(std::size_t size_bytes) : storage_(size_bytes) {}

auto GuestMemory::write(std::uint64_t guest_addr, std::span<const std::byte> data)
    -> core::Result<std::uint64_t> {
    if (guest_addr + data.size() > storage_.size()) {
        return core::make_error(core::ErrorCode::kOverflow,
                                "loader::GuestMemory: write out of bounds");
    }
    auto dst =
        std::span<std::byte>{storage_}.subspan(static_cast<std::size_t>(guest_addr), data.size());
    std::ranges::copy(data, dst.begin());
    ++epoch_;
    dirty_log_.push_back(DirtyRange{
        .guest_range_start = guest_addr,
        .guest_range_end = guest_addr + data.size(),
        .epoch = epoch_,
    });
    return epoch_;
}

auto GuestMemory::read(std::uint64_t guest_addr, std::size_t len) const noexcept
    -> core::Result<std::span<const std::byte>> {
    if (guest_addr + len > storage_.size()) {
        return core::make_error(core::ErrorCode::kOverflow,
                                "loader::GuestMemory: read out of bounds");
    }
    return std::span<const std::byte>{storage_}.subspan(static_cast<std::size_t>(guest_addr), len);
}

auto GuestMemory::is_range_dirty_after(std::uint64_t start, std::uint64_t end,
                                       std::uint64_t epoch) const noexcept -> bool {
    return std::ranges::any_of(dirty_log_, [&](const DirtyRange& entry) {
        return entry.epoch > epoch && entry.guest_range_start < end &&
               start < entry.guest_range_end;
    });
}

auto Loader::parse_synthetic_image(std::span<const std::byte> bytes)
    -> core::Result<SyntheticImage> {
    if (bytes.empty()) {
        return core::make_error(core::ErrorCode::kInvalidArgument,
                                "loader::Loader: empty synthetic image");
    }
    return SyntheticImage{
        .entry_point = 0x80000000ULL,
        .payload = std::vector<std::byte>(bytes.begin(), bytes.end()),
    };
}

auto Loader::load_commercial_nro(std::span<const std::byte> /*bytes*/) noexcept
    -> core::Result<SyntheticImage> {
    return core::make_error(core::ErrorCode::kNotImplemented,
                            "loader::Loader::load_commercial_nro not implemented");
}

} // namespace hikari::loader
