#include "cpu/cpu_backend.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <span>
#include <string>
#include <vector>

#include "core/hash.hpp"
#include "core/result.hpp"

namespace hikari::cpu {

namespace {

template <class T>
void hash_pod(core::Xxh3Stream& stream, const T& value) noexcept {
    static_assert(std::is_trivially_copyable_v<T>);
    std::array<std::byte, sizeof(T)> bytes{};
    std::memcpy(bytes.data(), &value, sizeof(T));
    stream.update(bytes);
}

[[nodiscard]] auto read_u32_le(std::span<const std::byte> mem,
                               std::uint64_t addr) noexcept -> core::Result<std::uint32_t> {
    if (addr > mem.size() || sizeof(std::uint32_t) > mem.size() - addr) {
        return core::make_error(core::ErrorCode::kInvalidArgument,
                                "guest memory read32 out of bounds");
    }
    std::uint32_t val = 0;
    std::memcpy(&val, mem.data() + addr, sizeof(val));
    return val;
}

[[nodiscard]] auto read_u64_le(std::span<const std::byte> mem,
                               std::uint64_t addr) noexcept -> core::Result<std::uint64_t> {
    if (addr > mem.size() || sizeof(std::uint64_t) > mem.size() - addr) {
        return core::make_error(core::ErrorCode::kInvalidArgument,
                                "guest memory read64 out of bounds");
    }
    std::uint64_t val = 0;
    std::memcpy(&val, mem.data() + addr, sizeof(val));
    return val;
}

[[nodiscard]] auto write_u64_le(std::span<std::byte> mem,
                                std::uint64_t addr,
                                std::uint64_t val) noexcept -> core::Result<void> {
    if (addr > mem.size() || sizeof(std::uint64_t) > mem.size() - addr) {
        return core::make_error(core::ErrorCode::kInvalidArgument,
                                "guest memory write64 out of bounds");
    }
    std::memcpy(mem.data() + addr, &val, sizeof(val));
    return {};
}

[[nodiscard]] auto sign_extend_19(std::uint32_t imm19) noexcept -> std::int64_t {
    const std::uint32_t masked = imm19 & 0x7FFFFU;
    if ((masked & 0x40000U) != 0U) {
        return static_cast<std::int64_t>(masked | 0xFFF80000U) |
               ~static_cast<std::int64_t>(0xFFFFFFFFULL);
    }
    return static_cast<std::int64_t>(masked);
}

[[nodiscard]] auto load_program_into_memory(std::span<std::byte> guest_mem,
                                            std::span<const std::uint32_t> program_words,
                                            std::uint64_t initial_pc) noexcept
    -> core::Result<void> {
    std::fill(guest_mem.begin(), guest_mem.end(), std::byte{0});
    const std::size_t bytes_needed = program_words.size() * sizeof(std::uint32_t);
    if (initial_pc > guest_mem.size() || bytes_needed > guest_mem.size() - initial_pc) {
        return core::make_error(core::ErrorCode::kInvalidArgument,
                                "program does not fit in guest memory");
    }
    if (!program_words.empty()) {
        std::memcpy(guest_mem.data() + initial_pc, program_words.data(), bytes_needed);
    }
    return {};
}

} // namespace

auto compute_block_hash(const BlockExecutionRecord& record) noexcept -> core::Hash128 {
    core::Xxh3Stream stream;
    hash_pod(stream, record.pc);
    hash_pod(stream, record.retired_instructions);
    for (const std::uint64_t reg : record.gpr) {
        hash_pod(stream, reg);
    }
    hash_pod(stream, record.sp);
    hash_pod(stream, record.nzcv);
    const auto range_count = static_cast<std::uint64_t>(record.written_ranges.size());
    hash_pod(stream, range_count);
    for (const auto& range : record.written_ranges) {
        hash_pod(stream, range.start);
        hash_pod(stream, range.length);
    }
    return stream.finalize();
}

DynarmicBackend::DynarmicBackend(CpuProfile profile, std::size_t guest_memory_bytes) noexcept
    : profile_(profile), guest_memory_(guest_memory_bytes, std::byte{0}) {}

auto DynarmicBackend::step_frame(std::span<const std::uint32_t> /*guest_words*/) noexcept
    -> core::Result<std::uint64_t> {
    return core::make_error(core::ErrorCode::kNotImplemented,
                            "cpu::DynarmicBackend::step_frame not implemented; use execute()");
}

auto DynarmicBackend::execute(std::span<const std::uint32_t> /*program_words*/,
                              std::uint64_t /*initial_pc*/,
                              const std::array<std::uint64_t, 31>& /*initial_gpr*/,
                              std::uint64_t /*initial_sp*/,
                              std::uint32_t /*initial_nzcv*/) noexcept
    -> core::Result<ExecutionTrace> {
    return core::make_error(core::ErrorCode::kNotImplemented,
                            "cpu::DynarmicBackend::execute not yet implemented");
}

auto DynarmicBackend::read_guest_u64(std::uint64_t addr) const noexcept
    -> core::Result<std::uint64_t> {
    return read_u64_le(guest_memory_, addr);
}

InterpreterBackend::InterpreterBackend(std::size_t guest_memory_bytes) noexcept
    : guest_memory_(guest_memory_bytes, std::byte{0}) {}

auto InterpreterBackend::step_frame(std::span<const std::uint32_t> /*guest_words*/) noexcept
    -> core::Result<std::uint64_t> {
    return core::make_error(core::ErrorCode::kNotImplemented,
                            "cpu::InterpreterBackend::step_frame not implemented; use execute()");
}

auto InterpreterBackend::read_guest_u64(std::uint64_t addr) const noexcept
    -> core::Result<std::uint64_t> {
    return read_u64_le(guest_memory_, addr);
}

auto InterpreterBackend::execute(std::span<const std::uint32_t> program_words,
                                 std::uint64_t initial_pc,
                                 const std::array<std::uint64_t, 31>& initial_gpr,
                                 std::uint64_t initial_sp,
                                 std::uint32_t initial_nzcv) noexcept
    -> core::Result<ExecutionTrace> {
    if (auto loaded = load_program_into_memory(guest_memory_, program_words, initial_pc);
        !loaded) {
        return std::unexpected(loaded.error());
    }

    ExecutionTrace trace{};
    trace.gpr = initial_gpr;
    trace.sp = initial_sp;
    trace.pc = initial_pc;
    trace.nzcv = initial_nzcv;

    constexpr std::uint64_t kMaxBlocks = 100000;
    bool halted = false;

    while (!halted) {
        if (trace.blocks.size() >= kMaxBlocks) {
            return core::make_error(core::ErrorCode::kOverflow,
                                    "interpreter exceeded maximum block count");
        }

        BlockExecutionRecord block{};
        block.pc = trace.pc;

        bool block_ended = false;
        while (!block_ended) {
            const auto insn_res = read_u32_le(guest_memory_, trace.pc);
            if (!insn_res) {
                return std::unexpected(insn_res.error());
            }
            const std::uint32_t insn = *insn_res;

            if ((insn & 0xFF800000U) == 0xD2800000U) {
                // MOVZ (64-bit): sf=1, opc=10
                const std::uint32_t hw = (insn >> 21U) & 0x3U;
                const std::uint64_t imm16 = (insn >> 5U) & 0xFFFFU;
                const std::uint32_t rd = insn & 0x1FU;
                if (rd != 31U) {
                    trace.gpr[rd] = imm16 << (hw * 16U);
                }
                trace.pc += 4U;
                ++block.retired_instructions;
            } else if ((insn & 0xFF200000U) == 0x8B000000U) {
                // ADD (shifted register, 64-bit, LSL): sf=1, op=0, S=0
                const std::uint32_t shift = (insn >> 22U) & 0x3U;
                if (shift != 0U) {
                    last_error_message_ =
                        std::format("unsupported ADD shift opcode 0x{:08X}", insn);
                    return core::make_error(core::ErrorCode::kNotImplemented, last_error_message_);
                }
                const std::uint32_t rm = (insn >> 16U) & 0x1FU;
                const std::uint32_t imm6 = (insn >> 10U) & 0x3FU;
                const std::uint32_t rn = (insn >> 5U) & 0x1FU;
                const std::uint32_t rd = insn & 0x1FU;
                const std::uint64_t lhs = (rn == 31U) ? 0ULL : trace.gpr[rn];
                const std::uint64_t rhs = ((rm == 31U) ? 0ULL : trace.gpr[rm]) << imm6;
                if (rd != 31U) {
                    trace.gpr[rd] = lhs + rhs;
                }
                trace.pc += 4U;
                ++block.retired_instructions;
            } else if ((insn & 0xFF800000U) == 0xD1000000U) {
                // SUB (immediate, 64-bit): sf=1, op=1, S=0
                const std::uint32_t sh = (insn >> 22U) & 0x1U;
                const std::uint64_t imm12 = (insn >> 10U) & 0xFFFU;
                const std::uint32_t rn = (insn >> 5U) & 0x1FU;
                const std::uint32_t rd = insn & 0x1FU;
                const std::uint64_t imm = imm12 << (sh != 0U ? 12U : 0U);
                const std::uint64_t lhs = (rn == 31U) ? trace.sp : trace.gpr[rn];
                const std::uint64_t result = lhs - imm;
                if (rd == 31U) {
                    trace.sp = result;
                } else {
                    trace.gpr[rd] = result;
                }
                trace.pc += 4U;
                ++block.retired_instructions;
            } else if ((insn & 0xFFC00000U) == 0xF9000000U) {
                // STR (unsigned offset, 64-bit)
                const std::uint64_t imm12 = (insn >> 10U) & 0xFFFU;
                const std::uint32_t rn = (insn >> 5U) & 0x1FU;
                const std::uint32_t rt = insn & 0x1FU;
                const std::uint64_t base = (rn == 31U) ? trace.sp : trace.gpr[rn];
                const std::uint64_t addr = base + (imm12 << 3U);
                const std::uint64_t value = (rt == 31U) ? 0ULL : trace.gpr[rt];
                if (auto wr = write_u64_le(guest_memory_, addr, value); !wr) {
                    return std::unexpected(wr.error());
                }
                block.written_ranges.push_back(MemoryWriteRange{.start = addr, .length = 8U});
                trace.pc += 4U;
                ++block.retired_instructions;
            } else if ((insn & 0xFF000000U) == 0xB5000000U) {
                // CBNZ (64-bit)
                const std::uint32_t imm19 = (insn >> 5U) & 0x7FFFFU;
                const std::uint32_t rt = insn & 0x1FU;
                const std::uint64_t value = (rt == 31U) ? 0ULL : trace.gpr[rt];
                if (value != 0U) {
                    const std::int64_t offset = sign_extend_19(imm19) * 4LL;
                    trace.pc = static_cast<std::uint64_t>(
                        static_cast<std::int64_t>(trace.pc) + offset);
                } else {
                    trace.pc += 4U;
                }
                ++block.retired_instructions;
                block_ended = true;
            } else if ((insn & 0xFFE0001FU) == 0xD4000001U) {
                // SVC #imm16
                const std::uint32_t imm16 = (insn >> 5U) & 0xFFFFU;
                if (imm16 != 0U) {
                    last_error_message_ =
                        std::format("unsupported SVC #{} (opcode 0x{:08X})", imm16, insn);
                    return core::make_error(core::ErrorCode::kNotImplemented, last_error_message_);
                }
                trace.pc += 4U;
                ++block.retired_instructions;
                block_ended = true;
                halted = true;
            } else {
                last_error_message_ =
                    std::format("unsupported A64 opcode 0x{:08X} at pc=0x{:X}", insn, trace.pc);
                return core::make_error(core::ErrorCode::kNotImplemented, last_error_message_);
            }
        }

        block.gpr = trace.gpr;
        block.sp = trace.sp;
        block.nzcv = trace.nzcv;
        block.hash = compute_block_hash(block);
        trace.total_instructions += block.retired_instructions;
        trace.blocks.push_back(std::move(block));
    }

    return trace;
}

} // namespace hikari::cpu
