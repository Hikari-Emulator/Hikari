#include <array>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <print>
#include <string>
#include <string_view>
#include <vector>

#include "capture/fixture.hpp"
#include "capture/reader.hpp"
#include "checkpoint/fault_recovery.hpp"
#include "core/assert.hpp"

namespace {

auto run_fault_suite(const std::vector<std::string_view>& args) -> int {
    const auto fix_a = hikari::capture::build_synthetic_fixture();
    HIKARI_CHECK(fix_a.has_value());
    const auto reader = hikari::capture::CaptureReader::open(*fix_a);
    HIKARI_CHECK(reader.has_value());

    const std::array<std::uint32_t, 4> rates = {0U, 1U, 5U, 20U};
    std::ofstream csv_out;
    if (args.size() == 3 && args.at(1) == "--write-csv") {
        csv_out.open(std::string{args.at(2)}, std::ios::trunc);
        HIKARI_CHECK(static_cast<bool>(csv_out));
        csv_out << "rate_percent,total_frames,injected_faults,detected_faults,rollbacks_completed,"
                   "unrecovered_mismatches,time_to_detect_frames,blast_radius_regions,"
                   "recovery_replayed_ops\n";
    }

    std::println(
        "rate_percent,total_frames,injected_faults,detected_faults,rollbacks_completed,unrecovered_"
        "mismatches,time_to_detect_frames,blast_radius_regions,recovery_replayed_ops");

    for (const std::uint32_t rate : rates) {
        const auto m = hikari::checkpoint::run_fault_injection_trial(*reader, rate, 100U);
        HIKARI_CHECK(m.has_value());
        HIKARI_CHECK(m->detected_faults == m->injected_faults);
        HIKARI_CHECK(m->rollbacks_completed == m->injected_faults);
        HIKARI_CHECK(m->unrecovered_mismatches == 0U);
        if (rate == 0U) {
            HIKARI_CHECK(m->injected_faults == 0U);
        } else {
            HIKARI_CHECK(m->injected_faults > 0U);
        }

        std::println("{},{},{},{},{},{},{},{},{}", m->fault_rate_percent, m->total_frames,
                     m->injected_faults, m->detected_faults, m->rollbacks_completed,
                     m->unrecovered_mismatches, m->mean_time_to_detect_frames,
                     m->max_blast_radius_regions, m->total_recovery_replayed_ops);

        if (csv_out.is_open()) {
            csv_out << m->fault_rate_percent << ',' << m->total_frames << ',' << m->injected_faults
                    << ',' << m->detected_faults << ',' << m->rollbacks_completed << ','
                    << m->unrecovered_mismatches << ',' << m->mean_time_to_detect_frames << ','
                    << m->max_blast_radius_regions << ',' << m->total_recovery_replayed_ops << '\n';
        }
    }

    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const std::vector<std::string_view> args(argv, std::next(argv, argc));
        return run_fault_suite(args);
    } catch (...) {
        return 1;
    }
}
