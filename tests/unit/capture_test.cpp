#include <array>
#include <cstddef>
#include <cstdint>
#include <print>
#include <vector>

#include "capture/fixture.hpp"
#include "capture/format.hpp"
#include "capture/reader.hpp"
#include "capture/writer.hpp"
#include "core/assert.hpp"
#include "core/id.hpp"
#include "core/result.hpp"

namespace {

void check_record(const hikari::capture::CaptureRecordView& rec, std::size_t& clear_count,
                  std::size_t& draw_count) {
    switch (rec.tag) {
    case hikari::capture::RecordTag::kBeginFrame: {
        const auto decoded = hikari::capture::decode_begin_frame(rec.payload);
        HIKARI_CHECK(decoded.has_value());
        break;
    }
    case hikari::capture::RecordTag::kClearOp: {
        const auto decoded = hikari::capture::decode_clear_op(rec.payload);
        HIKARI_CHECK(decoded.has_value());
        ++clear_count;
        break;
    }
    case hikari::capture::RecordTag::kDrawPatternOp: {
        const auto decoded = hikari::capture::decode_draw_pattern_op(rec.payload);
        HIKARI_CHECK(decoded.has_value());
        HIKARI_CHECK(decoded->second.size() == decoded->first.byte_length);
        ++draw_count;
        break;
    }
    case hikari::capture::RecordTag::kEndFrame: {
        const auto decoded = hikari::capture::decode_end_frame(rec.payload);
        HIKARI_CHECK(decoded.has_value());
        break;
    }
    case hikari::capture::RecordTag::kSampleOp: {
        const auto decoded = hikari::capture::decode_sample_op(rec.payload);
        HIKARI_CHECK(decoded.has_value());
        break;
    }
    }
}

void test_round_trip() {
    const auto fixture = hikari::capture::build_synthetic_fixture();
    HIKARI_CHECK(fixture.has_value());

    const auto reader = hikari::capture::CaptureReader::open(*fixture);
    HIKARI_CHECK(reader.has_value());
    HIKARI_CHECK(reader->header().format_version == hikari::capture::kCaptureFormatVersion);
    HIKARI_CHECK(reader->header().generation_id == hikari::capture::GenerationId::kMaxwell);
    HIKARI_CHECK(reader->header().frame_count == 3U);

    auto cursor = reader->cursor();
    std::size_t record_count = 0;
    std::size_t clear_count = 0;
    std::size_t draw_count = 0;

    while (true) {
        const auto rec = cursor.next();
        HIKARI_CHECK(rec.has_value());
        if (!*rec) {
            break;
        }
        ++record_count;
        check_record(**rec, clear_count, draw_count);
    }

    HIKARI_CHECK(record_count == 13U);
    HIKARI_CHECK(clear_count == 3U);
    HIKARI_CHECK(draw_count == 4U);
}

void test_flipped_byte_integrity() {
    const auto fixture = hikari::capture::build_synthetic_fixture();
    HIKARI_CHECK(fixture.has_value());

    std::vector<std::byte> corrupted = *fixture;
    const std::size_t target_idx = hikari::capture::kCaptureHeaderSize + 4U;
    corrupted.at(target_idx) ^= std::byte{0x01};

    const auto reader = hikari::capture::CaptureReader::open(corrupted);
    HIKARI_CHECK(!reader.has_value());
    HIKARI_CHECK(reader.error().code == hikari::core::ErrorCode::kIntegrityMismatch);
}

void test_malformed_headers() {
    const auto fixture = hikari::capture::build_synthetic_fixture();
    HIKARI_CHECK(fixture.has_value());

    // Bad magic.
    {
        std::vector<std::byte> bad_magic = *fixture;
        bad_magic.at(0) = std::byte{'X'};
        const auto reader = hikari::capture::CaptureReader::open(bad_magic);
        HIKARI_CHECK(!reader.has_value());
        HIKARI_CHECK(reader.error().code == hikari::core::ErrorCode::kInvalidMagic);
    }

    // Unknown version.
    {
        std::vector<std::byte> bad_ver = *fixture;
        bad_ver.at(4) = std::byte{0x63};
        const auto reader = hikari::capture::CaptureReader::open(bad_ver);
        HIKARI_CHECK(!reader.has_value());
        HIKARI_CHECK(reader.error().code == hikari::core::ErrorCode::kUnsupportedVersion);
    }

    // Truncated header.
    {
        const std::array<std::byte, 16> truncated{};
        const auto reader = hikari::capture::CaptureReader::open(truncated);
        HIKARI_CHECK(!reader.has_value());
        HIKARI_CHECK(reader.error().code == hikari::core::ErrorCode::kUnexpectedEof);
    }
}

} // namespace

int main() {
    try {
        test_round_trip();
        test_flipped_byte_integrity();
        test_malformed_headers();
        std::println("capture_test: all checks passed");
        return 0;
    } catch (...) {
        return 1;
    }
}
