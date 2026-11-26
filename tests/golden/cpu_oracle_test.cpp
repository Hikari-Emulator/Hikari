#include "cpu/cpu_backend.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string_view>

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

void print_register_dump(std::string_view label, const hikari::cpu::BlockExecutionRecord& rec) {
    std::cerr << "  [" << label << "] pc=0x" << std::hex << rec.pc << std::dec
              << " retired=" << rec.retired_instructions << " sp=0x" << std::hex << rec.sp
              << " nzcv=0x" << rec.nzcv << std::dec
              << " writes=" << rec.written_ranges.size() << "\n";
    for (std::size_t i = 0; i < rec.gpr.size(); ++i) {
        std::cerr << "    X" << i << "=0x" << std::hex << rec.gpr[i] << std::dec
                  << ((i % 4U == 3U || i + 1U == rec.gpr.size()) ? "\n" : " ");
    }
}

[[nodiscard]] bool run_and_compare(std::string_view program_name,
                                   std::span<const std::uint32_t> program,
                                   const hikari::cpu::CpuProfile& profile,
                                   const std::array<std::uint64_t, 31>& initial_gpr,
                                   std::uint64_t expected_x0,
                                   std::uint64_t expected_x1,
                                   std::uint64_t expected_x2,
                                   std::uint64_t expected_x3,
                                   std::uint64_t expected_mem_1000) {
    hikari::cpu::InterpreterBackend interp;
    hikari::cpu::DynarmicBackend dyn(profile);

    const auto interp_res = interp.execute(program, 0x0ULL, initial_gpr, 0ULL, 0U);
    if (!interp_res) {
        std::cerr << "Interpreter failed for " << program_name << ": "
                  << interp_res.error().message << "\n";
        return false;
    }

    const auto dyn_res = dyn.execute(program, 0x0ULL, initial_gpr, 0ULL, 0U);
    if (!dyn_res) {
        std::cerr << "Dynarmic failed for " << program_name << " profile=" << profile.name << ": "
                  << dyn_res.error().message << "\n";
        return false;
    }

    const auto& itrace = *interp_res;
    const auto& dtrace = *dyn_res;

    const std::size_t max_blocks = std::max(itrace.blocks.size(), dtrace.blocks.size());
    bool all_matched = (itrace.blocks.size() == dtrace.blocks.size());
    bool printed_first_mismatch = false;

    for (std::size_t i = 0; i < max_blocks; ++i) {
        const bool has_i = i < itrace.blocks.size();
        const bool has_d = i < dtrace.blocks.size();
        const bool match = has_i && has_d && (itrace.blocks[i].hash == dtrace.blocks[i].hash);

        const std::uint64_t pc = has_d ? dtrace.blocks[i].pc : itrace.blocks[i].pc;
        const auto dhex = has_d ? hikari::core::format_hash128(dtrace.blocks[i].hash)
                                : std::array<char, 33>{"<missing>"};
        const auto ihex = has_i ? hikari::core::format_hash128(itrace.blocks[i].hash)
                                : std::array<char, 33>{"<missing>"};

        std::cout << "program=" << program_name << " profile=" << profile.name << " block=" << i
                  << " pc=0x" << std::hex << pc << std::dec << " dynarmic=" << dhex.data()
                  << " interpreter=" << ihex.data()
                  << " status=" << (match ? "MATCH" : "MISMATCH") << "\n";

        if (!match) {
            all_matched = false;
            if (!printed_first_mismatch) {
                printed_first_mismatch = true;
                std::cerr << "First mismatch at program=" << program_name
                          << " profile=" << profile.name << " block=" << i << "\n";
                if (has_d) {
                    print_register_dump("dynarmic", dtrace.blocks[i]);
                }
                if (has_i) {
                    print_register_dump("interpreter", itrace.blocks[i]);
                }
            }
        }
    }

    if (!all_matched) {
        return false;
    }

    const auto check_final = [&](std::string_view backend_name,
                                 const hikari::cpu::ExecutionTrace& trace,
                                 std::uint64_t mem_1000) -> bool {
        if (trace.gpr[0] != expected_x0 || trace.gpr[1] != expected_x1 ||
            trace.gpr[2] != expected_x2 || trace.gpr[3] != expected_x3 || trace.sp != 0ULL ||
            trace.nzcv != 0U || mem_1000 != expected_mem_1000) {
            std::cerr << "Final state mismatch in " << backend_name << " for " << program_name
                      << " profile=" << profile.name << ": X0=" << trace.gpr[0]
                      << " X1=" << trace.gpr[1] << " X2=" << trace.gpr[2]
                      << " X3=0x" << std::hex << trace.gpr[3] << std::dec << " SP=" << trace.sp
                      << " NZCV=" << trace.nzcv << " mem[0x1000]=" << mem_1000 << "\n";
            return false;
        }
        return true;
    };

    const auto imem = interp.read_guest_u64(0x1000ULL);
    const auto dmem = dyn.read_guest_u64(0x1000ULL);
    if (!imem || !dmem) {
        std::cerr << "Failed reading guest memory at 0x1000\n";
        return false;
    }

    return check_final("interpreter", itrace, *imem) && check_final("dynarmic", dtrace, *dmem);
}

} // namespace

int main() {
    std::array<std::uint64_t, 31> p1_gpr{};
    p1_gpr[3] = 0x1000ULL;
    const std::array<std::uint64_t, 31> p2_gpr{};

    const std::array<hikari::cpu::CpuProfile, 2> profiles = {
        hikari::cpu::kSwitch1A57Profile,
        hikari::cpu::kSwitch2A78cProfile,
    };

    bool ok = true;
    for (const auto& profile : profiles) {
        ok = run_and_compare("P1", kProgramP1, profile, p1_gpr, 5ULL, 7ULL, 12ULL, 0x1000ULL,
                             12ULL) &&
             ok;
        ok = run_and_compare("P2", kProgramP2, profile, p2_gpr, 0ULL, 55ULL, 0ULL, 0ULL, 0ULL) &&
             ok;
    }

    return ok ? 0 : 1;
}
