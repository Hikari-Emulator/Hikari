#include "validate/reference_replay.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "capture/format.hpp"
#include "capture/reader.hpp"
#include "core/hash.hpp"
#include "core/id.hpp"
#include "core/result.hpp"

namespace hikari::validate {

namespace {

void hash_u32_le(core::Xxh3Stream& stream, std::uint32_t value) noexcept {
    const std::array<std::byte, 4> bytes = {
        static_cast<std::byte>(value & 0xFFU),
        static_cast<std::byte>((value >> 8U) & 0xFFU),
        static_cast<std::byte>((value >> 16U) & 0xFFU),
        static_cast<std::byte>((value >> 24U) & 0xFFU),
    };
    stream.update(bytes);
}

void hash_u64_le(core::Xxh3Stream& stream, std::uint64_t value) noexcept {
    std::array<std::byte, 8> bytes{};
    unsigned shift = 0;
    for (std::byte& b : bytes) {
        b = static_cast<std::byte>((value >> shift) & 0xFFU);
        shift += 8U;
    }
    stream.update(bytes);
}

struct ReplayState {
    core::Xxh3Stream frame_hasher;
    bool in_frame = false;
    core::FrameIndex active_frame{};
    std::size_t frame_count = 0;
    std::array<core::ResourceId, 32> written_resources{};
    std::size_t written_count = 0;
};

void record_written_resource(ReplayState& state, core::ResourceId res) noexcept {
    for (std::size_t i = 0; i < state.written_count; ++i) {
        if (state.written_resources.at(i) == res) {
            return;
        }
    }
    if (state.written_count < state.written_resources.size()) {
        state.written_resources.at(state.written_count++) = res;
    }
}

auto is_resource_written(const ReplayState& state, core::ResourceId res) noexcept -> bool {
    for (std::size_t i = 0; i < state.written_count; ++i) {
        if (state.written_resources.at(i) == res) {
            return true;
        }
    }
    return false;
}

auto handle_begin_frame(const capture::CaptureRecordView& rec, ReplayState& state) noexcept
    -> core::Result<void> {
    if (state.in_frame) {
        return core::make_error(core::ErrorCode::kInvalidArgument, "nested BeginFrame in capture");
    }
    const auto decoded = capture::decode_begin_frame(rec.payload);
    if (!decoded) {
        return std::unexpected{decoded.error()};
    }
    state.in_frame = true;
    state.active_frame = decoded->frame_index;
    state.frame_hasher.reset();
    hash_u32_le(state.frame_hasher, static_cast<std::uint32_t>(rec.tag));
    hash_u32_le(state.frame_hasher, rec.flags);
    hash_u64_le(state.frame_hasher, state.active_frame.value);
    return {};
}

auto handle_end_frame(const capture::CaptureRecordView& rec, ReplayState& state,
                      std::span<FrameSemanticHash> out_frames) noexcept -> core::Result<void> {
    if (!state.in_frame) {
        return core::make_error(core::ErrorCode::kInvalidArgument, "EndFrame without BeginFrame");
    }
    const auto decoded = capture::decode_end_frame(rec.payload);
    if (!decoded) {
        return std::unexpected{decoded.error()};
    }
    if (decoded->frame_index != state.active_frame) {
        return core::make_error(core::ErrorCode::kInvalidArgument, "EndFrame frame_index mismatch");
    }
    hash_u32_le(state.frame_hasher, static_cast<std::uint32_t>(rec.tag));
    hash_u32_le(state.frame_hasher, rec.flags);
    hash_u64_le(state.frame_hasher, decoded->frame_index.value);

    if (state.frame_count >= out_frames.size()) {
        return core::make_error(core::ErrorCode::kOverflow,
                                "out_frames buffer too small for capture");
    }
    out_frames.subspan(state.frame_count, 1).front() = FrameSemanticHash{
        .frame_index = state.active_frame,
        .semantic_content_hash = state.frame_hasher.finalize(),
    };
    ++state.frame_count;
    state.in_frame = false;
    return {};
}

auto process_record(const capture::CaptureRecordView& rec, ReplayState& state,
                    std::span<FrameSemanticHash> out_frames) noexcept -> core::Result<void> {
    switch (rec.tag) {
    case capture::RecordTag::kBeginFrame:
        return handle_begin_frame(rec, state);
    case capture::RecordTag::kClearOp: {
        if (!state.in_frame) {
            return core::make_error(core::ErrorCode::kInvalidArgument, "ClearOp outside of frame");
        }
        const auto decoded = capture::decode_clear_op(rec.payload);
        if (!decoded) {
            return std::unexpected{decoded.error()};
        }
        record_written_resource(state, decoded->target);
        hash_u32_le(state.frame_hasher, static_cast<std::uint32_t>(rec.tag));
        hash_u32_le(state.frame_hasher, rec.flags);
        state.frame_hasher.update(rec.payload);
        return {};
    }
    case capture::RecordTag::kDrawPatternOp: {
        if (!state.in_frame) {
            return core::make_error(core::ErrorCode::kInvalidArgument,
                                    "DrawPatternOp outside of frame");
        }
        const auto decoded = capture::decode_draw_pattern_op(rec.payload);
        if (!decoded) {
            return std::unexpected{decoded.error()};
        }
        record_written_resource(state, decoded->first.target);
        hash_u32_le(state.frame_hasher, static_cast<std::uint32_t>(rec.tag));
        hash_u32_le(state.frame_hasher, rec.flags);
        state.frame_hasher.update(rec.payload);
        return {};
    }
    case capture::RecordTag::kSampleOp: {
        if (!state.in_frame) {
            return core::make_error(core::ErrorCode::kInvalidArgument, "SampleOp outside of frame");
        }
        const auto decoded = capture::decode_sample_op(rec.payload);
        if (!decoded) {
            return std::unexpected{decoded.error()};
        }
        if (!is_resource_written(state, decoded->sampled)) {
            return core::make_error(core::ErrorCode::kInvalidArgument,
                                    "unresolved resource dependency in reference replay");
        }
        record_written_resource(state, decoded->target);
        hash_u32_le(state.frame_hasher, static_cast<std::uint32_t>(rec.tag));
        hash_u32_le(state.frame_hasher, rec.flags);
        state.frame_hasher.update(rec.payload);
        return {};
    }
    case capture::RecordTag::kEndFrame:
        return handle_end_frame(rec, state, out_frames);
    }
    return core::make_error(core::ErrorCode::kInvalidArgument, "unknown record tag");
}

} // namespace

auto replay_semantic_hashes(const capture::CaptureReader& reader,
                            std::span<FrameSemanticHash> out_frames) noexcept
    -> core::Result<std::size_t> {
    auto cursor = reader.cursor();
    ReplayState state{};

    while (true) {
        const auto rec_opt = cursor.next();
        if (!rec_opt) {
            return std::unexpected{rec_opt.error()};
        }
        if (!*rec_opt) {
            break;
        }
        const auto step = process_record(**rec_opt, state, out_frames);
        if (!step) {
            return std::unexpected{step.error()};
        }
    }

    if (state.in_frame) {
        return core::make_error(core::ErrorCode::kUnexpectedEof, "capture ended with open frame");
    }
    if (state.frame_count != reader.header().frame_count) {
        return core::make_error(core::ErrorCode::kInvalidArgument, "capture frame_count mismatch");
    }

    return state.frame_count;
}

auto replay_semantic_hashes(std::span<const std::byte> capture_bytes,
                            std::span<FrameSemanticHash> out_frames) noexcept
    -> core::Result<std::size_t> {
    const auto reader = capture::CaptureReader::open(capture_bytes);
    if (!reader) {
        return std::unexpected{reader.error()};
    }
    return replay_semantic_hashes(*reader, out_frames);
}

} // namespace hikari::validate
