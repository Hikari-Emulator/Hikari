#pragma once

#include <compare>
#include <cstdint>

namespace hikari::core {

struct ResourceId {
    std::uint32_t index = 0;
    std::uint32_t generation = 0;

    [[nodiscard]] constexpr bool operator==(const ResourceId&) const noexcept = default;
};

struct FrameIndex {
    std::uint64_t value = 0;

    [[nodiscard]] constexpr bool operator==(const FrameIndex&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const FrameIndex&) const noexcept = default;
};

static_assert(sizeof(ResourceId) == 8);
static_assert(sizeof(FrameIndex) == 8);

} // namespace hikari::core
