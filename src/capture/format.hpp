#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "core/id.hpp"

namespace hikari::capture {

inline constexpr std::array<std::uint8_t, 4> kCaptureMagic = {'H', 'K', 'C', 'P'};
inline constexpr std::uint32_t kCaptureFormatVersion = 1;
inline constexpr std::size_t kCaptureHeaderSize = 64;
inline constexpr std::size_t kRecordHeaderSize = 16;

enum class GenerationId : std::uint32_t {
    kMaxwell = 1,
    kAmpere = 2,
};

enum class RecordTag : std::uint32_t {
    kBeginFrame = 1,
    kClearOp = 2,
    kDrawPatternOp = 3,
    kEndFrame = 4,
    kSampleOp = 5,
};

struct CaptureHeader {
    std::array<std::uint8_t, 4> magic = kCaptureMagic;
    std::uint32_t format_version = kCaptureFormatVersion;
    GenerationId generation_id = GenerationId::kMaxwell;
    std::uint32_t reserved = 0;
    std::uint64_t frame_count = 0;
    std::uint64_t body_size = 0;
    std::array<std::uint8_t, 32> body_sha256{};
};

static_assert(sizeof(CaptureHeader) == kCaptureHeaderSize);

struct RecordHeader {
    RecordTag tag = RecordTag::kBeginFrame;
    std::uint32_t flags = 0;
    std::uint64_t payload_size = 0;
};

static_assert(sizeof(RecordHeader) == kRecordHeaderSize);

struct BeginFramePayload {
    core::FrameIndex frame_index{};
};

struct EndFramePayload {
    core::FrameIndex frame_index{};
};

struct ClearOpPayload {
    core::ResourceId target{};
    std::uint32_t rgba_value = 0;
    std::uint32_t flags = 0;
};

struct DrawPatternOpHeader {
    core::ResourceId target{};
    std::uint32_t offset = 0;
    std::uint32_t byte_length = 0;
};

struct SampleOpPayload {
    core::ResourceId target{};
    core::ResourceId sampled{};
    std::uint32_t flags = 0;
};

struct CaptureRecordView {
    RecordTag tag = RecordTag::kBeginFrame;
    std::uint32_t flags = 0;
    std::span<const std::byte> payload;
};

} // namespace hikari::capture
