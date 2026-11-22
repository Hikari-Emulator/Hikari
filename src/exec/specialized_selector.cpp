#include "exec/specialized_selector.hpp"

#include <cstddef>

#include "capture/format.hpp"
#include "core/hash.hpp"
#include "ir/ir_packet.hpp"

namespace hikari::exec {

auto classify_packet_signature(const ir::IrPacket& packet) noexcept -> StateSignatureKind {
    bool has_sample = false;
    bool has_clear_or_pattern = false;
    for (std::size_t i = 0; i < packet.operation_count; ++i) {
        const auto tag = packet.operations.at(i).tag;
        if (tag == capture::RecordTag::kSampleOp) {
            has_sample = true;
        } else if (tag == capture::RecordTag::kClearOp ||
                   tag == capture::RecordTag::kDrawPatternOp) {
            has_clear_or_pattern = true;
        }
    }
    if (has_sample) {
        return StateSignatureKind::kSampledRgba8;
    }
    if (has_clear_or_pattern) {
        return StateSignatureKind::kClearAndPatternRgba8;
    }
    return StateSignatureKind::kGenericFallback;
}

auto execute_packet_with_specialized_selection(const ir::IrPacket& packet) noexcept
    -> core::Hash128 {
    switch (classify_packet_signature(packet)) {
    case StateSignatureKind::kClearAndPatternRgba8:
        return execute_specialized_packet<StateSignatureKind::kClearAndPatternRgba8>(packet);
    case StateSignatureKind::kSampledRgba8:
        return execute_specialized_packet<StateSignatureKind::kSampledRgba8>(packet);
    case StateSignatureKind::kGenericFallback:
        return execute_specialized_packet<StateSignatureKind::kGenericFallback>(packet);
    }
    return execute_packet_generic(packet);
}

auto execute_packet_generic(const ir::IrPacket& packet) noexcept -> core::Hash128 {
    return ir::compute_ir_packet_semantic_hash(packet);
}

} // namespace hikari::exec
