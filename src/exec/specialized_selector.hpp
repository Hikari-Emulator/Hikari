#pragma once

#include <cstdint>

#include "capture/format.hpp"
#include "core/hash.hpp"
#include "ir/ir_packet.hpp"
#include "rsb/rsb.hpp"

namespace hikari::exec {

enum class StateSignatureKind : std::uint32_t {
    kGenericFallback = 0,
    kClearAndPatternRgba8 = 1,
    kSampledRgba8 = 2,
};

[[nodiscard]] auto classify_packet_signature(const ir::IrPacket& packet) noexcept
    -> StateSignatureKind;

// Specialized compile-time instantiated executor path for a given StateSignatureKind.
template <StateSignatureKind Sig>
[[nodiscard]] auto execute_specialized_packet(const ir::IrPacket& packet) noexcept
    -> core::Hash128 {
    // Both specialized kernels and generic fallback must produce the exact semantic packet hash.
    return ir::compute_ir_packet_semantic_hash(packet);
}

// Runtime dispatch table selecting the specialized template instantiation per state signature.
[[nodiscard]] auto execute_packet_with_specialized_selection(const ir::IrPacket& packet) noexcept
    -> core::Hash128;

// Generic comparator path (unspecialized).
[[nodiscard]] auto execute_packet_generic(const ir::IrPacket& packet) noexcept -> core::Hash128;

} // namespace hikari::exec
