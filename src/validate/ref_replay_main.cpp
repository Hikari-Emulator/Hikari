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
#include "validate/reference_replay.hpp"

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
    if (args.size() != 2) {
        std::println(stderr, "usage: hikari_ref_replay <capture>");
        return 2;
    }

    const std::vector<std::byte> bytes = read_file_bytes(args.at(1));
    if (bytes.empty()) {
        std::println(stderr, "ref_replay: failed to read {}", args.at(1));
        return 1;
    }

    const auto reader = hikari::capture::CaptureReader::open(bytes);
    if (!reader) {
        std::println(stderr, "ref_replay: open failed ({})",
                     static_cast<unsigned>(reader.error().code));
        return 1;
    }

    std::vector<hikari::validate::FrameSemanticHash> hashes(reader->header().frame_count);
    const auto count = hikari::validate::replay_semantic_hashes(bytes, hashes);
    if (!count) {
        std::println(stderr, "ref_replay: replay failed ({})",
                     static_cast<unsigned>(count.error().code));
        return 1;
    }

    for (std::size_t i = 0; i < *count; ++i) {
        const auto& entry = hashes.at(i);
        const auto hex = hikari::core::format_hash128(entry.semantic_content_hash);
        std::println("frame={} semantic_content_hash={}", entry.frame_index.value,
                     std::string_view{hex.data(), 32});
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
