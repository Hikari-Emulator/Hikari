#pragma once

#include <cstdint>
#include <string_view>

#include "core/result.hpp"

namespace hikari::services {

class ServiceHost {
public:
    virtual ~ServiceHost() noexcept = default;
    ServiceHost() noexcept = default;
    ServiceHost(const ServiceHost&) noexcept = default;
    auto operator=(const ServiceHost&) noexcept -> ServiceHost& = default;
    ServiceHost(ServiceHost&&) noexcept = default;
    auto operator=(ServiceHost&&) noexcept -> ServiceHost& = default;

    [[nodiscard]] virtual auto query_clock_ticks() const noexcept
        -> core::Result<std::uint64_t> = 0;
    [[nodiscard]] virtual auto open_filesystem(std::string_view mount_name) noexcept
        -> core::Result<std::uint32_t> = 0;
    [[nodiscard]] virtual auto poll_input() noexcept -> core::Result<std::uint32_t> = 0;
    [[nodiscard]] virtual auto submit_audio() noexcept -> core::Result<std::uint32_t> = 0;
};

class MinimalServiceHost final : public ServiceHost {
public:
    [[nodiscard]] auto query_clock_ticks() const noexcept -> core::Result<std::uint64_t> override;
    [[nodiscard]] auto open_filesystem(std::string_view mount_name) noexcept
        -> core::Result<std::uint32_t> override;
    [[nodiscard]] auto poll_input() noexcept -> core::Result<std::uint32_t> override;
    [[nodiscard]] auto submit_audio() noexcept -> core::Result<std::uint32_t> override;

private:
    std::uint64_t deterministic_ticks_ = 1000ULL;
};

} // namespace hikari::services
