#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/hash.hpp"
#include "core/id.hpp"
#include "core/result.hpp"
#include "ir/ir_packet.hpp"

namespace hikari::gfx {

class Pipeline {
public:
    explicit Pipeline(core::Hash128 key) noexcept : key_(key) {}
    [[nodiscard]] auto key() const noexcept -> core::Hash128 {
        return key_;
    }

private:
    core::Hash128 key_{};
};

class CommandBuffer {
public:
    explicit CommandBuffer(std::uint64_t packet_id) noexcept : packet_id_(packet_id) {}
    [[nodiscard]] auto packet_id() const noexcept -> std::uint64_t {
        return packet_id_;
    }

private:
    std::uint64_t packet_id_ = 0;
};

[[nodiscard]] auto submit_to_vulkan_device(const CommandBuffer& cmd_buf) noexcept
    -> core::Result<void>;

} // namespace hikari::gfx

namespace hikari::exec {

struct ExecutedFrameResult {
    std::uint64_t packet_id = 0;
    core::FrameIndex frame_index{};
    ir::PacketStatus status = ir::PacketStatus::kFinal;
    core::Hash128 frame_hash{};
    std::size_t pipeline_cache_hits = 0;
    std::size_t pipeline_cache_misses = 0;
};

class VulkanExecutor {
public:
    [[nodiscard]] auto consume_packet(const ir::IrPacket& packet)
        -> core::Result<ExecutedFrameResult>;

    [[nodiscard]] auto cached_pipeline_count() const noexcept -> std::size_t {
        return pipeline_cache_.size();
    }

private:
    std::uint64_t last_packet_id_ = 0;
    std::vector<core::Hash128> pipeline_cache_;
};

} // namespace hikari::exec
