#include <cstddef>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <print>
#include <string>
#include <string_view>
#include <vector>

#include "capture/reader.hpp"
#include "core/hash.hpp"
#include "runtime/frame_loop.hpp"

namespace {

auto read_file_bytes(std::string_view path) -> std::vector<std::byte> {
    std::ifstream in(std::string{path}, std::ios::binary | std::ios::ate);
    if (!in) {
        return {};
    }
    const auto end_pos = in.tellg();
    if (end_pos <= 0) {
        return {};
    }
    std::string raw(static_cast<std::size_t>(end_pos), '\0');
    in.seekg(0, std::ios::beg);
    if (!in.read(raw.data(), end_pos)) {
        return {};
    }
    std::vector<std::byte> out;
    out.reserve(raw.size());
    for (const char ch : raw) {
        out.push_back(static_cast<std::byte>(static_cast<unsigned char>(ch)));
    }
    return out;
}

auto run(const std::vector<std::string_view>& args) -> int {
    if (args.size() < 2 || args.size() > 3) {
        std::println(stderr, "usage: hikari_run <capture> [--threaded]");
        return 2;
    }

    const bool threaded = (args.size() == 3 && args.at(2) == "--threaded");
    const std::vector<std::byte> bytes = read_file_bytes(args.at(1));
    if (bytes.empty()) {
        std::println(stderr, "hikari_run: failed to read {}", args.at(1));
        return 1;
    }

    const hikari::runtime::RuntimeConfig cfg{
        .mode = threaded ? hikari::runtime::ExecutionMode::kThreadedSpsc
                         : hikari::runtime::ExecutionMode::kSingleThreaded,
        .seed = 0x5EED1234ULL,
    };

    const auto report = hikari::runtime::FrameLoop::run_capture(bytes, cfg);
    if (!report) {
        std::println(stderr, "hikari_run: failed ({})", static_cast<unsigned>(report.error().code));
        return 1;
    }

    for (const auto& cmp : report->comparisons) {
        const auto main_hex = hikari::core::format_hash128(cmp.main_hash);
        if (!report->reference_available) {
            std::println("frame={} main_hash={} reference_hash=ERROR status=reference error, "
                         "comparison not possible",
                         cmp.frame_index.value, std::string_view{main_hex.data(), 32});
        } else {
            const auto ref_hex = hikari::core::format_hash128(cmp.reference_hash);
            std::println("frame={} main_hash={} reference_hash={} status={}", cmp.frame_index.value,
                         std::string_view{main_hex.data(), 32},
                         std::string_view{ref_hex.data(), 32}, cmp.matched ? "MATCH" : "MISMATCH");
        }
    }

    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const std::vector<std::string_view> args(argv, std::next(argv, argc));
        return run(args);
    } catch (...) {
        return 1;
    }
}
