#include "services/service_host.hpp"

#include <cstdint>
#include <string_view>

#include "core/result.hpp"

namespace hikari::services {

auto MinimalServiceHost::query_clock_ticks() const noexcept -> core::Result<std::uint64_t> {
    return deterministic_ticks_;
}

auto MinimalServiceHost::open_filesystem(std::string_view /*mount_name*/) noexcept
    -> core::Result<std::uint32_t> {
    return core::make_error(core::ErrorCode::kNotImplemented,
                            "services::MinimalServiceHost::open_filesystem not implemented");
}

auto MinimalServiceHost::poll_input() noexcept -> core::Result<std::uint32_t> {
    return core::make_error(core::ErrorCode::kNotImplemented,
                            "services::MinimalServiceHost::poll_input not implemented");
}

auto MinimalServiceHost::submit_audio() noexcept -> core::Result<std::uint32_t> {
    return core::make_error(core::ErrorCode::kNotImplemented,
                            "services::MinimalServiceHost::submit_audio not implemented");
}

} // namespace hikari::services
