#include "capture/reader.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>

#include "core/hash.hpp"

namespace hikari::capture {

namespace {

auto read_u32_le(std::span<const std::byte> bytes) noexcept -> std::uint32_t {
    std::uint32_t value = 0;
    unsigned shift = 0;
    for (const std::byte b : bytes.subspan(0, 4)) {
        value |= static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(b)) << shift;
        shift += 8U;
    }
    return value;
}

auto read_u64_le(std::span<const std::byte> bytes) noexcept -> std::uint64_t {
    std::uint64_t value = 0;
    unsigned shift = 0;
    for (const std::byte b : bytes.subspan(0, 8)) {
        value |= static_cast<std::uint64_t>(std::to_integer<std::uint8_t>(b)) << shift;
        shift += 8U;
    }
    return value;
}

auto parse_record_tag(std::uint32_t raw) noexcept -> core::Result<RecordTag> {
    switch (raw) {
    case 1U:
        return RecordTag::kBeginFrame;
    case 2U:
        return RecordTag::kClearOp;
    case 3U:
        return RecordTag::kDrawPatternOp;
    case 4U:
        return RecordTag::kEndFrame;
    case 5U:
        return RecordTag::kSampleOp;
    default:
        return core::make_error(core::ErrorCode::kInvalidArgument, "unknown record tag");
    }
}

auto parse_generation_id(std::uint32_t raw) noexcept -> core::Result<GenerationId> {
    switch (raw) {
    case 1U:
        return GenerationId::kMaxwell;
    case 2U:
        return GenerationId::kAmpere;
    default:
        return core::make_error(core::ErrorCode::kUnsupportedVersion, "unknown GPU generation id");
    }
}

} // namespace

auto CaptureCursor::next() noexcept -> core::Result<std::optional<CaptureRecordView>> {
    if (remaining_.empty()) {
        return std::optional<CaptureRecordView>{std::nullopt};
    }
    if (remaining_.size() < kRecordHeaderSize) {
        return core::make_error(core::ErrorCode::kUnexpectedEof, "truncated record header");
    }

    const auto raw_tag = read_u32_le(remaining_.subspan(0, 4));
    const auto tag_result = parse_record_tag(raw_tag);
    if (!tag_result) {
        return std::unexpected{tag_result.error()};
    }

    const std::uint32_t flags = read_u32_le(remaining_.subspan(4, 4));
    const std::uint64_t payload_size = read_u64_le(remaining_.subspan(8, 8));

    const auto after_header = remaining_.subspan(kRecordHeaderSize);
    if (payload_size > after_header.size()) {
        return core::make_error(core::ErrorCode::kUnexpectedEof, "truncated record payload");
    }

    const auto payload_len = static_cast<std::size_t>(payload_size);
    const std::span<const std::byte> payload = after_header.subspan(0, payload_len);
    remaining_ = after_header.subspan(payload_len);

    return std::optional<CaptureRecordView>{
        CaptureRecordView{.tag = *tag_result, .flags = flags, .payload = payload}};
}

auto CaptureReader::open(std::span<const std::byte> data) noexcept -> core::Result<CaptureReader> {
    if (data.size() < kCaptureHeaderSize) {
        return core::make_error(core::ErrorCode::kUnexpectedEof, "capture smaller than header");
    }

    CaptureHeader header{};
    std::size_t magic_idx = 0;
    for (const std::byte b : data.subspan(0, 4)) {
        header.magic.at(magic_idx++) = std::to_integer<std::uint8_t>(b);
    }
    if (header.magic != kCaptureMagic) {
        return core::make_error(core::ErrorCode::kInvalidMagic, "invalid capture magic");
    }

    header.format_version = read_u32_le(data.subspan(4, 4));
    if (header.format_version != kCaptureFormatVersion) {
        return core::make_error(core::ErrorCode::kUnsupportedVersion,
                                "unsupported capture format version");
    }

    const auto gen_result = parse_generation_id(read_u32_le(data.subspan(8, 4)));
    if (!gen_result) {
        return std::unexpected{gen_result.error()};
    }
    header.generation_id = *gen_result;
    header.reserved = read_u32_le(data.subspan(12, 4));
    header.frame_count = read_u64_le(data.subspan(16, 8));
    header.body_size = read_u64_le(data.subspan(24, 8));

    const std::span<const std::byte> digest_bytes = data.subspan(32, 32);
    std::size_t digest_idx = 0;
    for (const std::byte b : digest_bytes) {
        header.body_sha256.at(digest_idx++) = std::to_integer<std::uint8_t>(b);
    }

    const std::span<const std::byte> body = data.subspan(kCaptureHeaderSize);
    if (header.body_size != body.size()) {
        return core::make_error(core::ErrorCode::kUnexpectedEof,
                                "capture body size does not match header");
    }

    const core::Sha256Digest actual_digest = core::sha256(body);
    if (!std::ranges::equal(actual_digest, header.body_sha256)) {
        return core::make_error(core::ErrorCode::kIntegrityMismatch,
                                "capture body SHA-256 mismatch");
    }

    return CaptureReader{header, body};
}

auto decode_begin_frame(std::span<const std::byte> payload) noexcept
    -> core::Result<BeginFramePayload> {
    if (payload.size() != 8) {
        return core::make_error(core::ErrorCode::kInvalidArgument,
                                "invalid BeginFrame payload size");
    }
    return BeginFramePayload{.frame_index = core::FrameIndex{.value = read_u64_le(payload)}};
}

auto decode_end_frame(std::span<const std::byte> payload) noexcept
    -> core::Result<EndFramePayload> {
    if (payload.size() != 8) {
        return core::make_error(core::ErrorCode::kInvalidArgument, "invalid EndFrame payload size");
    }
    return EndFramePayload{.frame_index = core::FrameIndex{.value = read_u64_le(payload)}};
}

auto decode_clear_op(std::span<const std::byte> payload) noexcept -> core::Result<ClearOpPayload> {
    if (payload.size() != 16) {
        return core::make_error(core::ErrorCode::kInvalidArgument, "invalid ClearOp payload size");
    }
    return ClearOpPayload{
        .target =
            core::ResourceId{
                .index = read_u32_le(payload.subspan(0, 4)),
                .generation = read_u32_le(payload.subspan(4, 4)),
            },
        .rgba_value = read_u32_le(payload.subspan(8, 4)),
        .flags = read_u32_le(payload.subspan(12, 4)),
    };
}

auto decode_draw_pattern_op(std::span<const std::byte> payload) noexcept
    -> core::Result<std::pair<DrawPatternOpHeader, std::span<const std::byte>>> {
    if (payload.size() < 16) {
        return core::make_error(core::ErrorCode::kInvalidArgument,
                                "invalid DrawPatternOp payload size");
    }
    const DrawPatternOpHeader header{
        .target =
            core::ResourceId{
                .index = read_u32_le(payload.subspan(0, 4)),
                .generation = read_u32_le(payload.subspan(4, 4)),
            },
        .offset = read_u32_le(payload.subspan(8, 4)),
        .byte_length = read_u32_le(payload.subspan(12, 4)),
    };
    const std::span<const std::byte> pattern = payload.subspan(16);
    if (pattern.size() != header.byte_length) {
        return core::make_error(core::ErrorCode::kInvalidArgument,
                                "DrawPatternOp byte_length mismatch");
    }
    return std::pair{header, pattern};
}

auto decode_sample_op(std::span<const std::byte> payload) noexcept
    -> core::Result<SampleOpPayload> {
    if (payload.size() != 20) {
        return core::make_error(core::ErrorCode::kInvalidArgument, "invalid SampleOp payload size");
    }
    return SampleOpPayload{
        .target =
            core::ResourceId{
                .index = read_u32_le(payload.subspan(0, 4)),
                .generation = read_u32_le(payload.subspan(4, 4)),
            },
        .sampled =
            core::ResourceId{
                .index = read_u32_le(payload.subspan(8, 4)),
                .generation = read_u32_le(payload.subspan(12, 4)),
            },
        .flags = read_u32_le(payload.subspan(16, 4)),
    };
}

} // namespace hikari::capture
