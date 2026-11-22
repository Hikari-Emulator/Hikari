#pragma once

#include <cstdint>
#include <span>

#include "core/result.hpp"

namespace hikari::cpu {

class CpuBackend {
public:
    virtual ~CpuBackend() noexcept = default;
    CpuBackend() noexcept = default;
    CpuBackend(const CpuBackend&) noexcept = default;
    auto operator=(const CpuBackend&) noexcept -> CpuBackend& = default;
    CpuBackend(CpuBackend&&) noexcept = default;
    auto operator=(CpuBackend&&) noexcept -> CpuBackend& = default;

    [[nodiscard]] virtual auto step_frame(std::span<const std::uint32_t> guest_words) noexcept
        -> core::Result<std::uint64_t> = 0;
};

class DynarmicBackend final : public CpuBackend {
public:
    [[nodiscard]] auto step_frame(std::span<const std::uint32_t> guest_words) noexcept
        -> core::Result<std::uint64_t> override;
};

class InterpreterBackend final : public CpuBackend {
public:
    [[nodiscard]] auto step_frame(std::span<const std::uint32_t> guest_words) noexcept
        -> core::Result<std::uint64_t> override;
};

} // namespace hikari::cpu
