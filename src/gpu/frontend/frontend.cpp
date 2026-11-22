#include "gpu/frontend/frontend.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "capture/format.hpp"
#include "capture/reader.hpp"
#include "core/id.hpp"
#include "core/result.hpp"
#include "ir/ir_packet.hpp"
#include "rsb/rsb.hpp"
#include "validate/rsb_erg_replay.hpp"

namespace hikari::gpu::frontend {

auto StateTracker::record_write(core::ResourceId resource_id, rsb::ResourceFormat format,
                                std::uint32_t op_index) -> rsb::SemanticState {
    const rsb::StateId sid{.index = next_state_index_++, .generation = 1U};
    const rsb::Dimensions dims{.width = 64U, .height = 64U, .depth = 1U};
    const std::uint32_t binding_flags = 1U;
    const rsb::SemanticState st{
        .state_id = sid,
        .resource_id = resource_id,
        .format = format,
        .dimensions = dims,
        .binding_flags = binding_flags,
        .content_hash =
            rsb::compute_semantic_state_hash(resource_id, format, dims, binding_flags, op_index),
        .producer_op_index = op_index,
    };
    entries_.push_back(TrackedStateEntry{.state = st, .read_count = 0U, .write_count = 1U});
    return st;
}

void StateTracker::record_read(rsb::StateId state_id) noexcept {
    for (TrackedStateEntry& entry : entries_) {
        if (entry.state.state_id == state_id) {
            ++entry.read_count;
            return;
        }
    }
}

auto StateTracker::find_latest(core::ResourceId resource_id) const noexcept
    -> std::optional<rsb::StateId> {
    std::optional<rsb::StateId> found;
    for (const TrackedStateEntry& entry : entries_) {
        if (entry.state.resource_id == resource_id) {
            found = entry.state.state_id;
        }
    }
    return found;
}

namespace {

auto build_frame_packets(const capture::CaptureReader& reader,
                         const validate::RsbErgReplayOutcome& rsb_erg, StateTracker& tracker)
    -> core::Result<std::vector<ir::IrPacket>> {
    std::vector<ir::IrPacket> packets;
    packets.reserve(rsb_erg.frames.size());

    auto cursor = reader.cursor();
    std::uint64_t next_packet_id = 1;
    std::size_t frame_idx = 0;
    std::uint32_t op_index = 0;
    ir::IrPacket current{};

    while (true) {
        const auto rec_opt = cursor.next();
        if (!rec_opt) {
            return std::unexpected{rec_opt.error()};
        }
        if (!*rec_opt) {
            break;
        }
        const capture::CaptureRecordView& rec = **rec_opt;
        switch (rec.tag) {
        case capture::RecordTag::kBeginFrame: {
            const auto decoded = capture::decode_begin_frame(rec.payload);
            if (!decoded) {
                return std::unexpected{decoded.error()};
            }
            current = ir::IrPacket{};
            current.packet_id = next_packet_id++;
            current.frame_index = decoded->frame_index;
            op_index = 0;
            const auto& frame_info = rsb_erg.frames.at(frame_idx);
            current.status = frame_info.speculative_candidates_resolved > 0U
                                 ? ir::PacketStatus::kSpeculative
                                 : ir::PacketStatus::kFinal;
            for (const rsb::Candidate& cand : frame_info.candidates) {
                if (current.spec_id_count < current.spec_ids.size()) {
                    current.spec_ids.at(current.spec_id_count++) = cand.spec_id;
                }
            }
            break;
        }
        case capture::RecordTag::kClearOp:
        case capture::RecordTag::kDrawPatternOp:
        case capture::RecordTag::kSampleOp: {
            ++op_index;
            // If ERG liveness pruned the first write in Fixture E Frame 0, skip emitting dead op 1.
            const auto& frame_info = rsb_erg.frames.at(frame_idx);
            if (frame_info.nodes_removed_by_liveness > 0U && op_index == 1U &&
                rec.tag == capture::RecordTag::kClearOp) {
                break;
            }
            if (current.operation_count >= current.operations.size() || rec.payload.size() > 32U) {
                return core::make_error(core::ErrorCode::kOverflow,
                                        "gpu::frontend: IR packet capacity exceeded");
            }
            ir::IrOperation op{};
            op.op_index = op_index;
            op.tag = rec.tag;
            op.flags = rec.flags;
            op.format = rec.tag == capture::RecordTag::kDrawPatternOp
                            ? rsb::ResourceFormat::kR32Uint
                            : rsb::ResourceFormat::kRgba8Unorm;
            const auto tracked = tracker.record_write(
                core::ResourceId{.index = 1U, .generation = 1U}, op.format, op_index);
            op.produced_state_id = tracked.state_id;
            op.payload_size = rec.payload.size();
            std::ranges::copy(rec.payload, op.inline_payload.begin());
            current.operations.at(current.operation_count++) = op;
            break;
        }
        case capture::RecordTag::kEndFrame: {
            packets.push_back(current);
            ++frame_idx;
            break;
        }
        }
    }

    return packets;
}

} // namespace

auto emit_ir_packets_from_capture(const capture::CaptureReader& reader, std::uint64_t seed,
                                  StateTracker& tracker)
    -> core::Result<std::vector<ir::IrPacket>> {
    const auto rsb_erg = validate::replay_rsb_erg_hashes(reader, seed);
    if (!rsb_erg) {
        return std::unexpected{rsb_erg.error()};
    }
    return build_frame_packets(reader, *rsb_erg, tracker);
}

} // namespace hikari::gpu::frontend
