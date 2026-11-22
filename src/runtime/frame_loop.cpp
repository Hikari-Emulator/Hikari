#include "runtime/frame_loop.hpp"

#include <cstddef>
#include <span>
#include <thread>
#include <vector>

#include "capture/reader.hpp"
#include "checkpoint/checkpoint.hpp"
#include "core/result.hpp"
#include "core/spsc_queue.hpp"
#include "exec/gfx/executor.hpp"
#include "gpu/frontend/frontend.hpp"
#include "ir/ir_packet.hpp"
#include "services/service_host.hpp"
#include "validate/reference_replay.hpp"
#include "validate/validator.hpp"

namespace hikari::runtime {

namespace {

auto execute_packets_single_thread(std::span<const ir::IrPacket> packets,
                                   exec::VulkanExecutor& executor, std::size_t& queue_stalls)
    -> core::Result<std::vector<exec::ExecutedFrameResult>> {
    core::SpscQueue<ir::IrPacket, 16> packet_queue;
    std::vector<exec::ExecutedFrameResult> results;
    results.reserve(packets.size());

    for (const ir::IrPacket& pkt : packets) {
        if (!packet_queue.try_push(pkt)) {
            ++queue_stalls;
            return core::make_error(core::ErrorCode::kOverflow,
                                    "runtime::FrameLoop: SPSC packet queue full");
        }
        const auto popped = packet_queue.try_pop();
        if (!popped) {
            return core::make_error(core::ErrorCode::kUnexpectedEof,
                                    "runtime::FrameLoop: SPSC packet queue empty");
        }
        const auto exec_res = executor.consume_packet(*popped);
        if (!exec_res) {
            return std::unexpected{exec_res.error()};
        }
        results.push_back(*exec_res);
    }
    return results;
}

auto execute_packets_threaded_spsc(std::span<const ir::IrPacket> packets,
                                   exec::VulkanExecutor& executor, std::size_t& queue_stalls)
    -> core::Result<std::vector<exec::ExecutedFrameResult>> {
    core::SpscQueue<ir::IrPacket, 16> packet_queue;
    std::vector<exec::ExecutedFrameResult> results;
    results.reserve(packets.size());
    core::Result<void> consumer_status{};

    std::jthread executor_thread([&]() {
        std::size_t consumed = 0;
        while (consumed < packets.size()) {
            const auto popped = packet_queue.try_pop();
            if (!popped) {
                std::this_thread::yield();
                continue;
            }
            const auto exec_res = executor.consume_packet(*popped);
            if (!exec_res) {
                consumer_status = std::unexpected{exec_res.error()};
                return;
            }
            results.push_back(*exec_res);
            ++consumed;
        }
    });

    for (const ir::IrPacket& pkt : packets) {
        while (!packet_queue.try_push(pkt)) {
            ++queue_stalls;
            std::this_thread::yield();
        }
    }

    executor_thread.join();
    if (!consumer_status) {
        return std::unexpected{consumer_status.error()};
    }
    return results;
}

} // namespace

auto FrameLoop::run_capture(const capture::CaptureReader& reader, const RuntimeConfig& config)
    -> core::Result<RuntimeRunReport> {
    services::MinimalServiceHost service_host;
    (void)service_host.query_clock_ticks();

    gpu::frontend::StateTracker tracker;
    const auto packets = gpu::frontend::emit_ir_packets_from_capture(reader, config.seed, tracker);
    if (!packets) {
        return std::unexpected{packets.error()};
    }

    exec::VulkanExecutor executor;
    std::size_t queue_stalls = 0;
    const auto executed = config.mode == ExecutionMode::kThreadedSpsc
                              ? execute_packets_threaded_spsc(*packets, executor, queue_stalls)
                              : execute_packets_single_thread(*packets, executor, queue_stalls);
    if (!executed) {
        return std::unexpected{executed.error()};
    }

    std::vector<validate::FrameSemanticHash> ref_hashes(reader.header().frame_count);
    const auto ref_res = validate::replay_semantic_hashes(reader, ref_hashes);

    RuntimeRunReport report{};
    report.reference_available = ref_res.has_value();
    report.metrics.frames_executed = executed->size();
    report.metrics.queue_stalls = queue_stalls;

    checkpoint::CheckpointManager checkpoints;
    for (std::size_t i = 0; i < executed->size(); ++i) {
        const auto& ex = executed->at(i);
        report.metrics.pipeline_cache_hits += ex.pipeline_cache_hits;
        report.metrics.pipeline_cache_misses += ex.pipeline_cache_misses;

        const core::Hash128 ref_hash =
            report.reference_available ? ref_hashes.at(i).semantic_content_hash : core::Hash128{};
        const auto cmp =
            validate::Validator::compare_frame(ex.frame_index, ex.frame_hash, ref_hash);
        if (report.reference_available && cmp.matched) {
            ++report.metrics.frames_matched;
            checkpoints.save_snapshot(ex.frame_index, ex.frame_hash);
        } else {
            ++report.metrics.frames_mismatched;
        }
        report.comparisons.push_back(cmp);
    }

    return report;
}

auto FrameLoop::run_capture(std::span<const std::byte> capture_bytes, const RuntimeConfig& config)
    -> core::Result<RuntimeRunReport> {
    const auto reader = capture::CaptureReader::open(capture_bytes);
    if (!reader) {
        return std::unexpected{reader.error()};
    }
    return run_capture(*reader, config);
}

} // namespace hikari::runtime
