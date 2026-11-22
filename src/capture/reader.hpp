#pragma once

#include <cstddef>
#include <optional>
#include <span>

#include "capture/format.hpp"
#include "core/result.hpp"

namespace hikari::capture {

class CaptureCursor {
public:
    explicit CaptureCursor(std::span<const std::byte> body) noexcept : remaining_(body) {}

    [[nodiscard]] auto next() noexcept -> core::Result<std::optional<CaptureRecordView>>;

private:
    std::span<const std::byte> remaining_;
};

class CaptureReader {
public:
    [[nodiscard]] static auto open(std::span<const std::byte> data) noexcept
        -> core::Result<CaptureReader>;

    [[nodiscard]] auto header() const noexcept -> const CaptureHeader& {
        return header_;
    }

    [[nodiscard]] auto body() const noexcept -> std::span<const std::byte> {
        return body_;
    }

    [[nodiscard]] auto cursor() const noexcept -> CaptureCursor {
        return CaptureCursor{body_};
    }

private:
    CaptureReader(CaptureHeader header, std::span<const std::byte> body) noexcept
        : header_(header), body_(body) {}

    CaptureHeader header_{};
    std::span<const std::byte> body_;
};

[[nodiscard]] auto decode_begin_frame(std::span<const std::byte> payload) noexcept
    -> core::Result<BeginFramePayload>;
[[nodiscard]] auto decode_end_frame(std::span<const std::byte> payload) noexcept
    -> core::Result<EndFramePayload>;
[[nodiscard]] auto decode_clear_op(std::span<const std::byte> payload) noexcept
    -> core::Result<ClearOpPayload>;
[[nodiscard]] auto decode_draw_pattern_op(std::span<const std::byte> payload) noexcept
    -> core::Result<std::pair<DrawPatternOpHeader, std::span<const std::byte>>>;
[[nodiscard]] auto decode_sample_op(std::span<const std::byte> payload) noexcept
    -> core::Result<SampleOpPayload>;

} // namespace hikari::capture
