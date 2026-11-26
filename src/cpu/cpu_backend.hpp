#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "core/hash.hpp"
#include "core/result.hpp"

namespace hikari::cpu {

struct MemoryWriteRange {
    std::uint64_t start = 0;
    std::uint64_t length = 0;
};

struct BlockExecutionRecord {
    std::uint64_t pc = 0;
    std::uint64_t retired_instructions = 0;
    std::array<std::uint64_t, 31> gpr{};
    std::uint64_t sp = 0;
    std::uint32_t nzcv = 0;
    std::vector<MemoryWriteRange> written_ranges;
    core::Hash128 hash{};
};

struct ExecutionTrace {
    std::array<std::uint64_t, 31> gpr{};
    std::uint64_t sp = 0;
    std::uint64_t pc = 0;
    std::uint32_t nzcv = 0;
    std::uint64_t total_instructions = 0;
    std::vector<BlockExecutionRecord> blocks;
};

struct CpuProfile {
    std::string_view name;
    std::uint32_t arch_level = 0;
    std::uint64_t features = 0;
};

// [HYPOTHESIS] the profile only sets the assumed guest feature set; not validated against hardware.
inline constexpr CpuProfile kSwitch1A57Profile{
    .name = "switch1_a57",
    .arch_level = 80,
    .features = 0x1ULL,
};

// [HYPOTHESIS] the profile only sets the assumed guest feature set; not validated against hardware.
inline constexpr CpuProfile kSwitch2A78cProfile{
    .name = "switch2_a78c",
    .arch_level = 82,
    .features = 0x3ULL,
};

[[nodiscard]] auto compute_block_hash(const BlockExecutionRecord& record) noexcept -> core::Hash128;

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
    explicit DynarmicBackend(CpuProfile profile = kSwitch1A57Profile,
                             std::size_t guest_memory_bytes = 64U * 1024U) noexcept;

    [[nodiscard]] auto step_frame(std::span<const std::uint32_t> guest_words) noexcept
        -> core::Result<std::uint64_t> override;

    [[nodiscard]] auto execute(std::span<const std::uint32_t> program_words,
                               std::uint64_t initial_pc,
                               const std::array<std::uint64_t, 31>& initial_gpr,
                               std::uint64_t initial_sp,
                               std::uint32_t initial_nzcv) noexcept
        -> core::Result<ExecutionTrace>;

    [[nodiscard]] auto read_guest_u64(std::uint64_t addr) const noexcept
        -> core::Result<std::uint64_t>;

    [[nodiscard]] auto profile() const noexcept -> const CpuProfile& { return profile_; }

private:
    CpuProfile profile_{};
    std::vector<std::byte> guest_memory_;
    mutable std::string last_error_message_;
};

class InterpreterBackend final : public CpuBackend {
public:
    explicit InterpreterBackend(std::size_t guest_memory_bytes = 64U * 1024U) noexcept;

    [[nodiscard]] auto step_frame(std::span<const std::uint32_t> guest_words) noexcept
        -> core::Result<std::uint64_t> override;

    [[nodiscard]] auto execute(std::span<const std::uint32_t> program_words,
                               std::uint64_t initial_pc,
                               const std::array<std::uint64_t, 31>& initial_gpr,
                               std::uint64_t initial_sp,
                               std::uint32_t initial_nzcv) noexcept
        -> core::Result<ExecutionTrace>;

    [[nodiscard]] auto read_guest_u64(std::uint64_t addr) const noexcept
        -> core::Result<std::uint64_t>;

private:
    std::vector<std::byte> guest_memory_;
    mutable std::string last_error_message_;
};

} // namespace hikari::cpu
