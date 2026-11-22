#include "capture/writer.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "core/assert.hpp"
#include "core/hash.hpp"

namespace hikari::capture {

namespace {

void write_u32_le_to(std::vector<std::byte>& out, std::uint32_t value) {
    out.push_back(static_cast<std::byte>(value & 0xFFU));
    out.push_back(static_cast<std::byte>((value >> 8U) & 0xFFU));
    out.push_back(static_cast<std::byte>((value >> 16U) & 0xFFU));
    out.push_back(static_cast<std::byte>((value >> 24U) & 0xFFU));
}

void write_u64_le_to(std::vector<std::byte>& out, std::uint64_t value) {
    for (unsigned shift = 0; shift < 64U; shift += 8U) {
        out.push_back(static_cast<std::byte>((value >> shift) & 0xFFU));
    }
}

} // namespace

CaptureWriter::CaptureWriter(GenerationId generation_id) : generation_id_(generation_id) {}

void CaptureWriter::begin_frame(core::FrameIndex frame_index) {
    HIKARI_ASSERT(!in_frame_);
    in_frame_ = true;
    active_frame_ = frame_index;
    append_record_header(RecordTag::kBeginFrame, 0, sizeof(std::uint64_t));
    append_u64_le(frame_index.value);
}

void CaptureWriter::emit_clear(core::ResourceId target, std::uint32_t rgba_value,
                               std::uint32_t flags) {
    HIKARI_ASSERT(in_frame_);
    constexpr std::uint64_t kPayloadBytes = 16;
    append_record_header(RecordTag::kClearOp, 0, kPayloadBytes);
    append_u32_le(target.index);
    append_u32_le(target.generation);
    append_u32_le(rgba_value);
    append_u32_le(flags);
}

void CaptureWriter::emit_draw_pattern(core::ResourceId target, std::uint32_t offset,
                                      std::span<const std::byte> pattern_bytes) {
    HIKARI_ASSERT(in_frame_);
    constexpr std::uint64_t kHeaderBytes = 16;
    const auto byte_length = static_cast<std::uint32_t>(pattern_bytes.size());
    append_record_header(RecordTag::kDrawPatternOp, 0, kHeaderBytes + byte_length);
    append_u32_le(target.index);
    append_u32_le(target.generation);
    append_u32_le(offset);
    append_u32_le(byte_length);
    append_bytes(pattern_bytes);
}

void CaptureWriter::end_frame(core::FrameIndex frame_index) {
    HIKARI_ASSERT(in_frame_);
    HIKARI_ASSERT(frame_index == active_frame_);
    in_frame_ = false;
    ++frame_count_;
    append_record_header(RecordTag::kEndFrame, 0, sizeof(std::uint64_t));
    append_u64_le(frame_index.value);
}

auto CaptureWriter::finalize() const -> core::Result<std::vector<std::byte>> {
    if (in_frame_) {
        return core::make_error(core::ErrorCode::kInvalidArgument,
                                "cannot finalize capture with an open frame");
    }

    const core::Sha256Digest digest = core::sha256(body_);

    std::vector<std::byte> out;
    out.reserve(kCaptureHeaderSize + body_.size());

    for (const std::uint8_t byte : kCaptureMagic) {
        out.push_back(static_cast<std::byte>(byte));
    }
    write_u32_le_to(out, kCaptureFormatVersion);
    write_u32_le_to(out, static_cast<std::uint32_t>(generation_id_));
    write_u32_le_to(out, 0U);
    write_u64_le_to(out, frame_count_);
    write_u64_le_to(out, static_cast<std::uint64_t>(body_.size()));
    for (const std::uint8_t byte : digest) {
        out.push_back(static_cast<std::byte>(byte));
    }

    out.insert(out.end(), body_.begin(), body_.end());
    return out;
}

void CaptureWriter::append_record_header(RecordTag tag, std::uint32_t flags,
                                         std::uint64_t payload_size) {
    append_u32_le(static_cast<std::uint32_t>(tag));
    append_u32_le(flags);
    append_u64_le(payload_size);
}

void CaptureWriter::append_u32_le(std::uint32_t value) {
    write_u32_le_to(body_, value);
}

void CaptureWriter::append_u64_le(std::uint64_t value) {
    write_u64_le_to(body_, value);
}

void CaptureWriter::append_bytes(std::span<const std::byte> bytes) {
    body_.insert(body_.end(), bytes.begin(), bytes.end());
}

} // namespace hikari::capture
