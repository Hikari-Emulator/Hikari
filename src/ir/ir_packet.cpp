#include "ir/ir_packet.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "capture/format.hpp"
#include "core/hash.hpp"

namespace hikari::ir {

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

} // namespace

auto compute_ir_packet_semantic_hash(const IrPacket& packet) noexcept -> core::Hash128 {
    core::Xxh3Stream stream;
    hash_u32_le(stream, static_cast<std::uint32_t>(capture::RecordTag::kBeginFrame));
    hash_u32_le(stream, 0U);
    hash_u64_le(stream, packet.frame_index.value);

    for (std::size_t i = 0; i < packet.operation_count; ++i) {
        const IrOperation& op = packet.operations.at(i);
        hash_u32_le(stream, static_cast<std::uint32_t>(op.tag));
        hash_u32_le(stream, op.flags);
        stream.update(std::span<const std::byte>{op.inline_payload}.subspan(0, op.payload_size));
    }

    hash_u32_le(stream, static_cast<std::uint32_t>(capture::RecordTag::kEndFrame));
    hash_u32_le(stream, 0U);
    hash_u64_le(stream, packet.frame_index.value);
    return stream.finalize();
}

} // namespace hikari::ir
