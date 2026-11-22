#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "capture/format.hpp"
#include "core/id.hpp"
#include "core/result.hpp"

namespace hikari::capture {

class CaptureWriter {
public:
    explicit CaptureWriter(GenerationId generation_id = GenerationId::kMaxwell);

    void begin_frame(core::FrameIndex frame_index);
    void emit_clear(core::ResourceId target, std::uint32_t rgba_value, std::uint32_t flags = 0);
    void emit_draw_pattern(core::ResourceId target, std::uint32_t offset,
                           std::span<const std::byte> pattern_bytes);
    void emit_sample(core::ResourceId target, core::ResourceId sampled, std::uint32_t flags = 0);
    void end_frame(core::FrameIndex frame_index);

    [[nodiscard]] auto finalize() const -> core::Result<std::vector<std::byte>>;

    [[nodiscard]] auto frame_count() const noexcept -> std::uint64_t {
        return frame_count_;
    }

private:
    void append_record_header(RecordTag tag, std::uint32_t flags, std::uint64_t payload_size);
    void append_u32_le(std::uint32_t value);
    void append_u64_le(std::uint64_t value);
    void append_bytes(std::span<const std::byte> bytes);

    GenerationId generation_id_ = GenerationId::kMaxwell;
    std::uint64_t frame_count_ = 0;
    bool in_frame_ = false;
    core::FrameIndex active_frame_{};
    std::vector<std::byte> body_;
};

} // namespace hikari::capture
