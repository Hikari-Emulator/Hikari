#include <cstddef>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "capture/fixture.hpp"
#include "capture/format.hpp"
#include "capture/reader.hpp"
#include "core/hash.hpp"

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

auto write_file_bytes(std::string_view path, std::span<const std::byte> data) -> bool {
    std::ofstream out(std::string{path}, std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }
    std::string raw;
    raw.reserve(data.size());
    for (const std::byte b : data) {
        raw.push_back(std::to_integer<char>(b));
    }
    out.write(raw.data(), static_cast<std::streamsize>(raw.size()));
    return static_cast<bool>(out);
}

auto tag_name(hikari::capture::RecordTag tag) noexcept -> std::string_view {
    switch (tag) {
    case hikari::capture::RecordTag::kBeginFrame:
        return "BeginFrame";
    case hikari::capture::RecordTag::kClearOp:
        return "ClearOp";
    case hikari::capture::RecordTag::kDrawPatternOp:
        return "DrawPatternOp";
    case hikari::capture::RecordTag::kEndFrame:
        return "EndFrame";
    }
    return "Unknown";
}

auto write_fixture_option(std::string_view flag, std::string_view out_path) -> int {
    hikari::core::Result<std::vector<std::byte>> fixture =
        hikari::core::make_error(hikari::core::ErrorCode::kInvalidArgument, "unknown flag");
    if (flag == "--write-synthetic") {
        fixture = hikari::capture::build_synthetic_fixture();
    } else if (flag == "--write-barrier-reordered") {
        fixture = hikari::capture::build_barrier_reordered_fixture();
    } else if (flag == "--write-empty-frame") {
        fixture = hikari::capture::build_empty_frame_fixture();
    }
    if (!fixture) {
        std::println(stderr, "capdump: failed to build fixture for {}", flag);
        return 1;
    }
    if (!write_file_bytes(out_path, *fixture)) {
        std::println(stderr, "capdump: failed to write {}", out_path);
        return 1;
    }
    std::println("wrote {} bytes to {}", fixture->size(), out_path);
    return 0;
}

auto run(const std::vector<std::string_view>& args) -> int {
    if (args.size() == 3 &&
        (args.at(1) == "--write-synthetic" || args.at(1) == "--write-barrier-reordered" ||
         args.at(1) == "--write-empty-frame")) {
        return write_fixture_option(args.at(1), args.at(2));
    }

    if (args.size() != 2) {
        std::println(stderr, "usage: hikari_capdump [--write-synthetic|--write-barrier-reordered|"
                             "--write-empty-frame <out>] | <capture>");
        return 2;
    }

    const std::vector<std::byte> bytes = read_file_bytes(args.at(1));
    if (bytes.empty()) {
        std::println(stderr, "capdump: failed to read {}", args.at(1));
        return 1;
    }

    const auto reader = hikari::capture::CaptureReader::open(bytes);
    if (!reader) {
        std::println(stderr, "capdump: open failed ({})",
                     static_cast<unsigned>(reader.error().code));
        return 1;
    }

    const auto& hdr = reader->header();
    const auto sha_hex = hikari::core::format_sha256(hdr.body_sha256);
    std::println("version={} generation={} frames={} body_bytes={} sha256={}", hdr.format_version,
                 static_cast<unsigned>(hdr.generation_id), hdr.frame_count, hdr.body_size,
                 std::string_view{sha_hex.data(), 64});

    auto cursor = reader->cursor();
    std::size_t index = 0;
    while (true) {
        const auto rec = cursor.next();
        if (!rec) {
            std::println(stderr, "capdump: record {} error", index);
            return 1;
        }
        if (!*rec) {
            break;
        }
        std::println("  [{}] {} flags={} payload_bytes={}", index, tag_name((*rec)->tag),
                     (*rec)->flags, (*rec)->payload.size());
        ++index;
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
