#include "validate/rsb_erg_replay.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "capture/format.hpp"
#include "capture/reader.hpp"
#include "core/hash.hpp"
#include "core/id.hpp"
#include "core/result.hpp"
#include "erg/erg.hpp"
#include "erg/hct_stub.hpp"
#include "rsb/rsb.hpp"
#include "validate/reference_replay.hpp"

namespace hikari::validate {

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

struct FrameOpRecord {
    std::uint32_t op_index = 0;
    capture::RecordTag tag = capture::RecordTag::kBeginFrame;
    std::uint32_t flags = 0;
    std::span<const std::byte> payload;
    rsb::StateId produced_state_id{};
};

struct ResourceProducerEntry {
    core::ResourceId resource_id{};
    rsb::StateId latest_state_id{};
};

auto find_latest_state(std::span<const ResourceProducerEntry> table,
                       core::ResourceId resource_id) noexcept -> std::optional<rsb::StateId> {
    for (const ResourceProducerEntry& entry : table) {
        if (entry.resource_id == resource_id) {
            return entry.latest_state_id;
        }
    }
    return std::nullopt;
}

void upsert_latest_state(std::vector<ResourceProducerEntry>& table, core::ResourceId resource_id,
                         rsb::StateId state_id) {
    for (ResourceProducerEntry& entry : table) {
        if (entry.resource_id == resource_id) {
            entry.latest_state_id = state_id;
            return;
        }
    }
    table.push_back(ResourceProducerEntry{.resource_id = resource_id, .latest_state_id = state_id});
}

void add_known_format(std::vector<rsb::ResourceFormat>& formats, rsb::ResourceFormat fmt) {
    if (std::ranges::find(formats, fmt) == formats.end()) {
        formats.push_back(fmt);
    }
}

struct FrameBuildContext {
    capture::GenerationId generation = capture::GenerationId::kMaxwell;
    std::uint64_t seed = 0;
    std::uint64_t next_spec_id = 1;
    std::uint32_t next_state_index = 1;
    std::vector<ResourceProducerEntry> latest_producers;
    std::vector<rsb::ResourceFormat> capture_known_formats;
};

auto make_produced_state(FrameBuildContext& ctx, core::ResourceId target, rsb::ResourceFormat fmt,
                         std::uint32_t op_index) -> rsb::SemanticState {
    const rsb::StateId sid{.index = ctx.next_state_index++, .generation = 1U};
    const rsb::Dimensions dims{.width = 64U, .height = 64U, .depth = 1U};
    const std::uint32_t binding_flags = 1U;
    add_known_format(ctx.capture_known_formats, fmt);
    upsert_latest_state(ctx.latest_producers, target, sid);
    return rsb::SemanticState{
        .state_id = sid,
        .resource_id = target,
        .format = fmt,
        .dimensions = dims,
        .binding_flags = binding_flags,
        .content_hash =
            rsb::compute_semantic_state_hash(target, fmt, dims, binding_flags, op_index),
        .producer_op_index = op_index,
    };
}

auto resolve_or_bind_sample(FrameBuildContext& ctx, erg::Erg& graph,
                            std::vector<rsb::Candidate>& frame_candidates, core::ResourceId sampled,
                            std::uint32_t op_index) -> core::Result<void> {
    const auto existing = find_latest_state(ctx.latest_producers, sampled);
    if (existing) {
        graph.add_dependency(rsb::Dependency{
            .consumer_op_index = op_index,
            .required_state_id = *existing,
            .required_resource_id = sampled,
        });
        return {};
    }

    const rsb::StateId missing_state_id{.index = ctx.next_state_index++, .generation = 1U};
    const rsb::Dependency missing_dep{
        .consumer_op_index = op_index,
        .required_state_id = missing_state_id,
        .required_resource_id = sampled,
    };
    const rsb::ResolveContext rsb_ctx{
        .seed = ctx.seed,
        .next_spec_id = ctx.next_spec_id++,
        .assumed_resource_version = 0U,
        .default_dimensions = rsb::Dimensions{.width = 64U, .height = 64U, .depth = 1U},
        .default_binding_flags = 1U,
        .capture_known_formats = ctx.capture_known_formats,
        .hct_allowed_formats = erg::hct_allowed_formats(ctx.generation),
    };

    const auto candidate = rsb::Rsb::resolve(missing_dep, rsb_ctx);
    if (!candidate) {
        return std::unexpected{candidate.error()};
    }

    frame_candidates.push_back(*candidate);
    graph.add_node(erg::ErgNode{
        .state = candidate->state,
        .has_side_effect = false,
        .speculative = candidate->speculative,
        .spec_id = candidate->spec_id,
        .hotness = 1,
        .relevance_score = 0,
    });
    graph.add_dependency(missing_dep);
    upsert_latest_state(ctx.latest_producers, sampled, candidate->state.state_id);
    return {};
}

auto finalize_frame_erg(core::FrameIndex active_frame, capture::GenerationId generation,
                        const std::vector<FrameOpRecord>& frame_ops, erg::Erg& graph,
                        std::vector<rsb::Candidate>& frame_candidates)
    -> core::Result<RsbErgFrameOutcome> {
    if (!graph.check_hardware_bound(generation)) {
        return core::make_error(core::ErrorCode::kInvalidArgument,
                                "ERG hardware bound check failed");
    }

    const std::size_t before_liveness = graph.nodes().size();
    const std::size_t removed_by_liveness = graph.run_liveness_pass();
    const std::size_t after_liveness = graph.nodes().size();
    (void)graph.compute_relevance_ordering();

    const auto is_state_alive = [&](rsb::StateId sid) noexcept -> bool {
        return std::ranges::any_of(
            graph.nodes(), [&](const erg::ErgNode& node) { return node.state.state_id == sid; });
    };

    core::Xxh3Stream hasher;
    hash_u32_le(hasher, static_cast<std::uint32_t>(capture::RecordTag::kBeginFrame));
    hash_u32_le(hasher, 0U);
    hash_u64_le(hasher, active_frame.value);

    for (const FrameOpRecord& op : frame_ops) {
        if (!is_state_alive(op.produced_state_id)) {
            continue;
        }
        hash_u32_le(hasher, static_cast<std::uint32_t>(op.tag));
        hash_u32_le(hasher, op.flags);
        hasher.update(op.payload);
    }

    hash_u32_le(hasher, static_cast<std::uint32_t>(capture::RecordTag::kEndFrame));
    hash_u32_le(hasher, 0U);
    hash_u64_le(hasher, active_frame.value);

    return RsbErgFrameOutcome{
        .frame_hash =
            FrameSemanticHash{
                .frame_index = active_frame,
                .semantic_content_hash = hasher.finalize(),
            },
        .nodes_before_liveness = before_liveness,
        .nodes_removed_by_liveness = removed_by_liveness,
        .nodes_after_liveness = after_liveness,
        .speculative_candidates_resolved = frame_candidates.size(),
        .candidates = std::move(frame_candidates),
    };
}

struct RsbErgLoopState {
    FrameBuildContext ctx;
    RsbErgReplayOutcome outcome;
    bool in_frame = false;
    core::FrameIndex active_frame{};
    std::uint32_t op_index = 0;
    erg::Erg frame_graph;
    std::vector<FrameOpRecord> frame_ops;
    std::vector<rsb::Candidate> frame_candidates;
};

auto process_rsb_erg_record(const capture::CaptureRecordView& rec, RsbErgLoopState& st)
    -> core::Result<void> {
    switch (rec.tag) {
    case capture::RecordTag::kBeginFrame: {
        const auto decoded = capture::decode_begin_frame(rec.payload);
        if (!decoded || st.in_frame) {
            return core::make_error(core::ErrorCode::kInvalidArgument, "invalid BeginFrame");
        }
        st.in_frame = true;
        st.active_frame = decoded->frame_index;
        st.op_index = 0;
        st.frame_graph = erg::Erg{};
        st.frame_ops.clear();
        st.frame_candidates.clear();
        return {};
    }
    case capture::RecordTag::kClearOp: {
        const auto decoded = capture::decode_clear_op(rec.payload);
        if (!decoded || !st.in_frame) {
            return core::make_error(core::ErrorCode::kInvalidArgument, "invalid ClearOp");
        }
        ++st.op_index;
        const auto produced = make_produced_state(st.ctx, decoded->target,
                                                  rsb::ResourceFormat::kRgba8Unorm, st.op_index);
        st.frame_graph.add_node(erg::ErgNode{.state = produced, .hotness = 10});
        st.frame_ops.push_back(FrameOpRecord{.op_index = st.op_index,
                                             .tag = rec.tag,
                                             .flags = rec.flags,
                                             .payload = rec.payload,
                                             .produced_state_id = produced.state_id});
        return {};
    }
    case capture::RecordTag::kDrawPatternOp: {
        const auto decoded = capture::decode_draw_pattern_op(rec.payload);
        if (!decoded || !st.in_frame) {
            return core::make_error(core::ErrorCode::kInvalidArgument, "invalid DrawPatternOp");
        }
        ++st.op_index;
        // DrawPatternOp writes a subrange at offset; if the target resource already has a prior
        // state, this partial write reads/modifies that prior state rather than overwriting it.
        const auto prior_state = find_latest_state(st.ctx.latest_producers, decoded->first.target);
        if (prior_state) {
            st.frame_graph.add_dependency(rsb::Dependency{
                .consumer_op_index = st.op_index,
                .required_state_id = *prior_state,
                .required_resource_id = decoded->first.target,
            });
        }
        const auto produced = make_produced_state(st.ctx, decoded->first.target,
                                                  rsb::ResourceFormat::kR32Uint, st.op_index);
        st.frame_graph.add_node(erg::ErgNode{.state = produced,
                                             .has_side_effect = true,
                                             .overwrites_in_full = false,
                                             .reads_target = true,
                                             .hotness = 20});
        st.frame_ops.push_back(FrameOpRecord{.op_index = st.op_index,
                                             .tag = rec.tag,
                                             .flags = rec.flags,
                                             .payload = rec.payload,
                                             .produced_state_id = produced.state_id});
        return {};
    }
    case capture::RecordTag::kSampleOp: {
        const auto decoded = capture::decode_sample_op(rec.payload);
        if (!decoded || !st.in_frame) {
            return core::make_error(core::ErrorCode::kInvalidArgument, "invalid SampleOp");
        }
        ++st.op_index;
        const auto res = resolve_or_bind_sample(st.ctx, st.frame_graph, st.frame_candidates,
                                                decoded->sampled, st.op_index);
        if (!res) {
            return std::unexpected{res.error()};
        }
        const auto produced = make_produced_state(st.ctx, decoded->target,
                                                  rsb::ResourceFormat::kRgba8Unorm, st.op_index);
        st.frame_graph.add_node(erg::ErgNode{
            .state = produced,
            .overwrites_in_full = false,
            .reads_target = true,
            .hotness = 30,
        });
        st.frame_ops.push_back(FrameOpRecord{.op_index = st.op_index,
                                             .tag = rec.tag,
                                             .flags = rec.flags,
                                             .payload = rec.payload,
                                             .produced_state_id = produced.state_id});
        return {};
    }
    case capture::RecordTag::kEndFrame: {
        const auto decoded = capture::decode_end_frame(rec.payload);
        if (!decoded || !st.in_frame || decoded->frame_index != st.active_frame) {
            return core::make_error(core::ErrorCode::kInvalidArgument, "invalid EndFrame");
        }
        auto frame_res = finalize_frame_erg(st.active_frame, st.ctx.generation, st.frame_ops,
                                            st.frame_graph, st.frame_candidates);
        if (!frame_res) {
            return std::unexpected{frame_res.error()};
        }
        st.outcome.frames.push_back(std::move(*frame_res));
        st.in_frame = false;
        return {};
    }
    }
    return core::make_error(core::ErrorCode::kInvalidArgument, "unknown record tag");
}

} // namespace

auto replay_rsb_erg_hashes(const capture::CaptureReader& reader, std::uint64_t seed)
    -> core::Result<RsbErgReplayOutcome> {
    auto cursor = reader.cursor();
    RsbErgLoopState st{};
    st.ctx.generation = reader.header().generation_id;
    st.ctx.seed = seed;

    while (true) {
        const auto rec_opt = cursor.next();
        if (!rec_opt) {
            return std::unexpected{rec_opt.error()};
        }
        if (!*rec_opt) {
            break;
        }
        const auto step = process_rsb_erg_record(**rec_opt, st);
        if (!step) {
            return std::unexpected{step.error()};
        }
    }

    if (st.in_frame || st.outcome.frames.size() != reader.header().frame_count) {
        return core::make_error(core::ErrorCode::kInvalidArgument, "incomplete frames in capture");
    }
    return std::move(st.outcome);
}

auto replay_rsb_erg_hashes(std::span<const std::byte> capture_bytes, std::uint64_t seed)
    -> core::Result<RsbErgReplayOutcome> {
    const auto reader = capture::CaptureReader::open(capture_bytes);
    if (!reader) {
        return std::unexpected{reader.error()};
    }
    return replay_rsb_erg_hashes(*reader, seed);
}

} // namespace hikari::validate
