#include "cpu/cpu_backend.hpp"

#include <cstdint>
#include <span>

#include "core/result.hpp"

namespace hikari::cpu {

auto DynarmicBackend::step_frame(std::span<const std::uint32_t> /*guest_words*/) noexcept
    -> core::Result<std::uint64_t> {
    return core::make_error(core::ErrorCode::kNotImplemented,
                            "cpu::DynarmicBackend not implemented (Dynarmic not vendored)");
}

auto InterpreterBackend::step_frame(std::span<const std::uint32_t> /*guest_words*/) noexcept
    -> core::Result<std::uint64_t> {
    return core::make_error(core::ErrorCode::kNotImplemented,
                            "cpu::InterpreterBackend not implemented");
}

} // namespace hikari::cpu
