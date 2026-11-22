#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "capture/format.hpp"
#include "core/hash.hpp"
#include "core/id.hpp"
#include "core/result.hpp"
#include "rsb/rsb.hpp"

namespace hikari::ir {

inline constexpr std::uint16_t kIrVersionMajor = 1;
inline constexpr std::uint16_t kIrVersionMinor = 0;

enum class PacketStatus : std::uint32_t {
    kFinal = 1,
    kSpeculative = 2,
};

struct IrOperation {
    std::uint32_t op_index = 0;
    capture::RecordTag tag = capture::RecordTag::kBeginFrame;
    std::uint32_t flags = 0;
    rsb::StateId produced_state_id{};
    rsb::ResourceFormat format = rsb::ResourceFormat::kRgba8Unorm;
    std::array<std::byte, 32> inline_payload{};
    std::size_t payload_size = 0;
};

struct IrPacket {
    std::uint64_t packet_id = 0;
    std::uint16_t version_major = kIrVersionMajor;
    std::uint16_t version_minor = kIrVersionMinor;
    core::FrameIndex frame_index{};
    PacketStatus status = PacketStatus::kFinal;
    std::array<std::uint64_t, 8> spec_ids{};
    std::size_t spec_id_count = 0;
    std::array<IrOperation, 16> operations{};
    std::size_t operation_count = 0;
};

static_assert(sizeof(IrPacket) > 0);

[[nodiscard]] auto compute_ir_packet_semantic_hash(const IrPacket& packet) noexcept
    -> core::Hash128;

} // namespace hikari::ir
