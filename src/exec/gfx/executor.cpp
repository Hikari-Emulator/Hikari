#include "exec/gfx/executor.hpp"

#include <algorithm>
#include <cstddef>

#include "core/hash.hpp"
#include "core/result.hpp"
#include "ir/ir_packet.hpp"

namespace hikari::gfx {

auto submit_to_vulkan_device(const CommandBuffer& /*cmd_buf*/) noexcept -> core::Result<void> {
    return core::make_error(core::ErrorCode::kNotImplemented,
                            "gfx::submit_to_vulkan_device not implemented (headless mode)");
}

} // namespace hikari::gfx

namespace hikari::exec {

auto VulkanExecutor::consume_packet(const ir::IrPacket& packet)
    -> core::Result<ExecutedFrameResult> {
    if (packet.version_major != ir::kIrVersionMajor) {
        return core::make_error(core::ErrorCode::kUnsupportedVersion,
                                "exec::VulkanExecutor: unknown IR major version");
    }
    if (packet.packet_id <= last_packet_id_) {
        return core::make_error(core::ErrorCode::kInvalidArgument,
                                "exec::VulkanExecutor: out-of-order IR packet_id");
    }
    last_packet_id_ = packet.packet_id;

    std::size_t hits = 0;
    std::size_t misses = 0;
    for (std::size_t i = 0; i < packet.operation_count; ++i) {
        const ir::IrOperation& op = packet.operations.at(i);
        const core::Hash128 key = core::xxh3_128(
            std::span<const std::byte>{op.inline_payload}.subspan(0, op.payload_size));
        if (std::ranges::find(pipeline_cache_, key) != pipeline_cache_.end()) {
            ++hits;
        } else {
            pipeline_cache_.push_back(key);
            ++misses;
        }
    }

    return ExecutedFrameResult{
        .packet_id = packet.packet_id,
        .frame_index = packet.frame_index,
        .status = packet.status,
        .frame_hash = ir::compute_ir_packet_semantic_hash(packet),
        .pipeline_cache_hits = hits,
        .pipeline_cache_misses = misses,
    };
}

} // namespace hikari::exec
