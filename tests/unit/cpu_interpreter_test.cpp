#include "cpu/cpu_backend.hpp"

#include <array>
#include <cstdint>
#include <iostream>

#include "core/assert.hpp"
#include "core/hash.hpp"
#include "core/result.hpp"

namespace {

constexpr std::array<std::uint32_t, 5> kProgramP1 = {
    0xD28000A0U, // MOVZ X0, #5
    0xD28000E1U, // MOVZ X1, #7
    0x8B010002U, // ADD X2, X0, X1
    0xF9000062U, // STR X2, [X3]
    0xD4000001U, // SVC #0
};

constexpr std::array<std::uint32_t, 6> kProgramP2 = {
    0xD2800140U, // MOVZ X0, #10
    0xD2800001U, // MOVZ X1, #0
    0x8B000021U, // ADD X1, X1, X0
    0xD1000400U, // SUB X0, X0, #1
    0xB5FFFFC0U, // CBNZ X0, #-8
    0xD4000001U, // SVC #0
};

void test_p1() {
    hikari::cpu::InterpreterBackend interp;
    std::array<std::uint64_t, 31> gpr{};
    gpr[3] = 0x1000ULL;

    const auto res = interp.execute(kProgramP1, 0x0ULL, gpr, 0ULL, 0U);
    HIKARI_CHECK(res.has_value());

    const auto& trace = *res;
    HIKARI_CHECK(trace.gpr[0] == 5ULL);
    HIKARI_CHECK(trace.gpr[1] == 7ULL);
    HIKARI_CHECK(trace.gpr[2] == 12ULL);
    HIKARI_CHECK(trace.gpr[3] == 0x1000ULL);
    HIKARI_CHECK(trace.sp == 0ULL);
    HIKARI_CHECK(trace.nzcv == 0U);
    HIKARI_CHECK(trace.blocks.size() == 1U);
    HIKARI_CHECK(trace.total_instructions == 5ULL);

    const auto mem_res = interp.read_guest_u64(0x1000ULL);
    HIKARI_CHECK(mem_res.has_value());
    HIKARI_CHECK(*mem_res == 12ULL);

    const auto hex = hikari::core::format_hash128(trace.blocks[0].hash);
    std::cout << "P1: blocks=" << trace.blocks.size()
              << " insns=" << trace.total_instructions
              << " X0=" << trace.gpr[0]
              << " X1=" << trace.gpr[1]
              << " X2=" << trace.gpr[2]
              << " X3=0x" << std::hex << trace.gpr[3] << std::dec
              << " mem[0x1000]=" << *mem_res
              << " block0_hash=" << hex.data() << "\n";
}

void test_p2() {
    hikari::cpu::InterpreterBackend interp;
    const std::array<std::uint64_t, 31> gpr{};

    const auto res = interp.execute(kProgramP2, 0x0ULL, gpr, 0ULL, 0U);
    HIKARI_CHECK(res.has_value());

    const auto& trace = *res;
    HIKARI_CHECK(trace.gpr[0] == 0ULL);
    HIKARI_CHECK(trace.gpr[1] == 55ULL);
    HIKARI_CHECK(trace.sp == 0ULL);
    HIKARI_CHECK(trace.nzcv == 0U);
    HIKARI_CHECK(trace.blocks.size() == 11U);
    HIKARI_CHECK(trace.total_instructions == 33ULL);

    const auto hex = hikari::core::format_hash128(trace.blocks.back().hash);
    std::cout << "P2: blocks=" << trace.blocks.size()
              << " insns=" << trace.total_instructions
              << " X0=" << trace.gpr[0]
              << " X1=" << trace.gpr[1]
              << " last_block_hash=" << hex.data() << "\n";
}

void test_unsupported_opcode() {
    hikari::cpu::InterpreterBackend interp;
    const std::array<std::uint32_t, 1> bad_prog = {0xD503201FU}; // NOP
    const std::array<std::uint64_t, 31> gpr{};

    const auto res = interp.execute(bad_prog, 0x0ULL, gpr, 0ULL, 0U);
    HIKARI_CHECK(!res.has_value());
    HIKARI_CHECK(res.error().code == hikari::core::ErrorCode::kNotImplemented);
    std::cout << "Unsupported opcode error: " << res.error().message << "\n";
}

} // namespace

int main() {
    test_p1();
    test_p2();
    test_unsupported_opcode();
    return 0;
}
