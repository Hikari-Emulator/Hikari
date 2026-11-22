#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "capture/format.hpp"
#include "capture/reader.hpp"
#include "core/id.hpp"
#include "core/result.hpp"
#include "erg/erg.hpp"
#include "gpu/gen/generations.hpp"
#include "ir/ir_packet.hpp"
#include "rsb/rsb.hpp"

namespace hikari::gpu::frontend {

struct TrackedStateEntry {
    rsb::SemanticState state{};
    std::uint32_t read_count = 0;
    std::uint32_t write_count = 1;
};

class StateTracker {
public:
    auto record_write(core::ResourceId resource_id, rsb::ResourceFormat format,
                      std::uint32_t op_index) -> rsb::SemanticState;
    void record_read(rsb::StateId state_id) noexcept;

    [[nodiscard]] auto find_latest(core::ResourceId resource_id) const noexcept
        -> std::optional<rsb::StateId>;
    [[nodiscard]] auto entries() const noexcept -> std::span<const TrackedStateEntry> {
        return entries_;
    }

private:
    std::uint32_t next_state_index_ = 1;
    std::vector<TrackedStateEntry> entries_;
};

template <gpu::gen::GpuGeneration Gen> class GpuFrontend {
public:
    [[nodiscard]] static constexpr auto generation_name() noexcept -> std::string_view {
        return Gen::kName;
    }

    [[nodiscard]] static auto decode_word(const typename Gen::CommandWord* word,
                                          gpu::gen::CaptureSink& sink) noexcept
        -> core::Result<gpu::gen::DecodeResult> {
        return Gen::decode(word, sink);
    }
};

[[nodiscard]] auto emit_ir_packets_from_capture(const capture::CaptureReader& reader,
                                                std::uint64_t seed, StateTracker& tracker)
    -> core::Result<std::vector<ir::IrPacket>>;

} // namespace hikari::gpu::frontend
