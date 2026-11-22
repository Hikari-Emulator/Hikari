#include "capture/fixture.hpp"

#include <array>
#include <cstddef>
#include <vector>

#include "capture/format.hpp"
#include "capture/writer.hpp"
#include "core/id.hpp"
#include "core/result.hpp"

namespace hikari::capture {

auto build_synthetic_fixture() -> core::Result<std::vector<std::byte>> {
    CaptureWriter writer{GenerationId::kMaxwell};

    const core::ResourceId color_target{.index = 1U, .generation = 1U};
    const core::ResourceId vertex_buf{.index = 2U, .generation = 1U};

    // Frame 0: clear to opaque blue, draw 4-byte pattern.
    writer.begin_frame(core::FrameIndex{.value = 0U});
    writer.emit_clear(color_target, 0xFF0000FFU, 0U);
    const std::array<std::byte, 4> pat0 = {
        std::byte{0x10},
        std::byte{0x20},
        std::byte{0x30},
        std::byte{0x40},
    };
    writer.emit_draw_pattern(vertex_buf, 0U, pat0);
    writer.end_frame(core::FrameIndex{.value = 0U});

    // Frame 1: clear to opaque green, draw 8-byte pattern at offset 16.
    writer.begin_frame(core::FrameIndex{.value = 1U});
    writer.emit_clear(color_target, 0xFF00FF00U, 1U);
    const std::array<std::byte, 8> pat1 = {
        std::byte{0xAA}, std::byte{0xBB}, std::byte{0xCC}, std::byte{0xDD},
        std::byte{0x01}, std::byte{0x02}, std::byte{0x03}, std::byte{0x04},
    };
    writer.emit_draw_pattern(vertex_buf, 16U, pat1);
    writer.end_frame(core::FrameIndex{.value = 1U});

    // Frame 2: clear to opaque red, two draw patterns.
    writer.begin_frame(core::FrameIndex{.value = 2U});
    writer.emit_clear(color_target, 0xFFFF0000U, 0U);
    const std::array<std::byte, 4> pat2a = {
        std::byte{0xDE},
        std::byte{0xAD},
        std::byte{0xBE},
        std::byte{0xEF},
    };
    const std::array<std::byte, 4> pat2b = {
        std::byte{0xCA},
        std::byte{0xFE},
        std::byte{0xBA},
        std::byte{0xBE},
    };
    writer.emit_draw_pattern(vertex_buf, 32U, pat2a);
    writer.emit_draw_pattern(color_target, 64U, pat2b);
    writer.end_frame(core::FrameIndex{.value = 2U});

    return writer.finalize();
}

auto build_barrier_reordered_fixture() -> core::Result<std::vector<std::byte>> {
    CaptureWriter writer{GenerationId::kMaxwell};

    const core::ResourceId color_target{.index = 1U, .generation = 1U};
    const core::ResourceId vertex_buf{.index = 2U, .generation = 1U};

    // Frame 0: identical to build_synthetic_fixture().
    writer.begin_frame(core::FrameIndex{.value = 0U});
    writer.emit_clear(color_target, 0xFF0000FFU, 0U);
    const std::array<std::byte, 4> pat0 = {
        std::byte{0x10},
        std::byte{0x20},
        std::byte{0x30},
        std::byte{0x40},
    };
    writer.emit_draw_pattern(vertex_buf, 0U, pat0);
    writer.end_frame(core::FrameIndex{.value = 0U});

    // Frame 1: identical to build_synthetic_fixture().
    writer.begin_frame(core::FrameIndex{.value = 1U});
    writer.emit_clear(color_target, 0xFF00FF00U, 1U);
    const std::array<std::byte, 8> pat1 = {
        std::byte{0xAA}, std::byte{0xBB}, std::byte{0xCC}, std::byte{0xDD},
        std::byte{0x01}, std::byte{0x02}, std::byte{0x03}, std::byte{0x04},
    };
    writer.emit_draw_pattern(vertex_buf, 16U, pat1);
    writer.end_frame(core::FrameIndex{.value = 1U});

    // Frame 2: barrier/order moved — color_target draw emitted before vertex_buf draw.
    writer.begin_frame(core::FrameIndex{.value = 2U});
    writer.emit_clear(color_target, 0xFFFF0000U, 0U);
    const std::array<std::byte, 4> pat2a = {
        std::byte{0xDE},
        std::byte{0xAD},
        std::byte{0xBE},
        std::byte{0xEF},
    };
    const std::array<std::byte, 4> pat2b = {
        std::byte{0xCA},
        std::byte{0xFE},
        std::byte{0xBA},
        std::byte{0xBE},
    };
    writer.emit_draw_pattern(color_target, 64U, pat2b);
    writer.emit_draw_pattern(vertex_buf, 32U, pat2a);
    writer.end_frame(core::FrameIndex{.value = 2U});

    return writer.finalize();
}

auto build_empty_frame_fixture() -> core::Result<std::vector<std::byte>> {
    CaptureWriter writer{GenerationId::kMaxwell};

    const core::ResourceId color_target{.index = 1U, .generation = 1U};
    const core::ResourceId vertex_buf{.index = 2U, .generation = 1U};

    // Frame 0: identical to build_synthetic_fixture().
    writer.begin_frame(core::FrameIndex{.value = 0U});
    writer.emit_clear(color_target, 0xFF0000FFU, 0U);
    const std::array<std::byte, 4> pat0 = {
        std::byte{0x10},
        std::byte{0x20},
        std::byte{0x30},
        std::byte{0x40},
    };
    writer.emit_draw_pattern(vertex_buf, 0U, pat0);
    writer.end_frame(core::FrameIndex{.value = 0U});

    // Frame 1: zero operations between BeginFrame and EndFrame.
    writer.begin_frame(core::FrameIndex{.value = 1U});
    writer.end_frame(core::FrameIndex{.value = 1U});

    // Frame 2: identical to build_synthetic_fixture().
    writer.begin_frame(core::FrameIndex{.value = 2U});
    writer.emit_clear(color_target, 0xFFFF0000U, 0U);
    const std::array<std::byte, 4> pat2a = {
        std::byte{0xDE},
        std::byte{0xAD},
        std::byte{0xBE},
        std::byte{0xEF},
    };
    const std::array<std::byte, 4> pat2b = {
        std::byte{0xCA},
        std::byte{0xFE},
        std::byte{0xBA},
        std::byte{0xBE},
    };
    writer.emit_draw_pattern(vertex_buf, 32U, pat2a);
    writer.emit_draw_pattern(color_target, 64U, pat2b);
    writer.end_frame(core::FrameIndex{.value = 2U});

    return writer.finalize();
}

auto build_incomplete_dependency_fixture() -> core::Result<std::vector<std::byte>> {
    CaptureWriter writer{GenerationId::kMaxwell};

    const core::ResourceId color_target{.index = 1U, .generation = 1U};
    const core::ResourceId unbound_sampled{.index = 3U, .generation = 1U};

    writer.begin_frame(core::FrameIndex{.value = 0U});
    writer.emit_clear(color_target, 0xFF0000FFU, 0U);
    writer.emit_sample(color_target, unbound_sampled, 0U);
    writer.end_frame(core::FrameIndex{.value = 0U});

    return writer.finalize();
}

auto build_dead_write_fixture() -> core::Result<std::vector<std::byte>> {
    CaptureWriter writer{GenerationId::kMaxwell};

    const core::ResourceId color_target{.index = 1U, .generation = 1U};
    const core::ResourceId vertex_buf{.index = 2U, .generation = 1U};

    // Frame 0: dead clear to 0x11223344 immediately overwritten by clear to 0xFF0000FF.
    writer.begin_frame(core::FrameIndex{.value = 0U});
    writer.emit_clear(color_target, 0x11223344U, 0U);
    writer.emit_clear(color_target, 0xFF0000FFU, 0U);
    const std::array<std::byte, 4> pat0 = {
        std::byte{0x10},
        std::byte{0x20},
        std::byte{0x30},
        std::byte{0x40},
    };
    writer.emit_draw_pattern(vertex_buf, 0U, pat0);
    writer.end_frame(core::FrameIndex{.value = 0U});

    // Frame 1: identical to build_synthetic_fixture().
    writer.begin_frame(core::FrameIndex{.value = 1U});
    writer.emit_clear(color_target, 0xFF00FF00U, 1U);
    const std::array<std::byte, 8> pat1 = {
        std::byte{0xAA}, std::byte{0xBB}, std::byte{0xCC}, std::byte{0xDD},
        std::byte{0x01}, std::byte{0x02}, std::byte{0x03}, std::byte{0x04},
    };
    writer.emit_draw_pattern(vertex_buf, 16U, pat1);
    writer.end_frame(core::FrameIndex{.value = 1U});

    // Frame 2: identical to build_synthetic_fixture().
    writer.begin_frame(core::FrameIndex{.value = 2U});
    writer.emit_clear(color_target, 0xFFFF0000U, 0U);
    const std::array<std::byte, 4> pat2a = {
        std::byte{0xDE},
        std::byte{0xAD},
        std::byte{0xBE},
        std::byte{0xEF},
    };
    const std::array<std::byte, 4> pat2b = {
        std::byte{0xCA},
        std::byte{0xFE},
        std::byte{0xBA},
        std::byte{0xBE},
    };
    writer.emit_draw_pattern(vertex_buf, 32U, pat2a);
    writer.emit_draw_pattern(color_target, 64U, pat2b);
    writer.end_frame(core::FrameIndex{.value = 2U});

    return writer.finalize();
}

} // namespace hikari::capture
