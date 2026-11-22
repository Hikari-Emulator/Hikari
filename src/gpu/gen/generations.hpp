#pragma once

#include <concepts>
#include <cstdint>
#include <span>
#include <string_view>

#include "capture/format.hpp"
#include "core/result.hpp"

namespace hikari::gpu::gen {

struct HctTable {
    capture::GenerationId generation = capture::GenerationId::kMaxwell;
    std::uint32_t max_surface_dimension = 16384U;
};

struct DecodeResult {
    std::uint32_t decoded_ops = 0;
};

struct CaptureSink {
    std::uint32_t accepted_ops = 0;
};

template <class G>
concept GpuGeneration = requires {
    typename G::CommandWord;
    typename G::CaptureEntry;
    { G::kName } -> std::convertible_to<std::string_view>;
    { G::kHctTable } -> std::convertible_to<const HctTable&>;
    {
        G::decode(std::declval<const typename G::CommandWord*>(), std::declval<CaptureSink&>())
    } -> std::same_as<core::Result<DecodeResult>>;
};

struct MaxwellGen {
    using CommandWord = std::uint32_t;
    using CaptureEntry = capture::CaptureRecordView;
    static constexpr std::string_view kName = "Maxwell";
    static constexpr HctTable kHctTable{
        .generation = capture::GenerationId::kMaxwell,
        .max_surface_dimension = 16384U,
    };

    [[nodiscard]] static auto decode(const CommandWord* word, CaptureSink& sink) noexcept
        -> core::Result<DecodeResult>;
};

struct AmpereGen {
    using CommandWord = std::uint32_t;
    using CaptureEntry = capture::CaptureRecordView;
    static constexpr std::string_view kName = "Ampere";
    static constexpr HctTable kHctTable{
        .generation = capture::GenerationId::kAmpere,
        .max_surface_dimension = 32768U,
    };

    [[nodiscard]] static auto decode(const CommandWord* word, CaptureSink& sink) noexcept
        -> core::Result<DecodeResult>;
};

static_assert(GpuGeneration<MaxwellGen>);
static_assert(GpuGeneration<AmpereGen>);

} // namespace hikari::gpu::gen
