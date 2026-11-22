#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "capture/reader.hpp"
#include "core/result.hpp"
#include "rsb/rsb.hpp"
#include "validate/reference_replay.hpp"

namespace hikari::validate {

struct RsbErgFrameOutcome {
    FrameSemanticHash frame_hash{};
    std::size_t nodes_before_liveness = 0;
    std::size_t nodes_removed_by_liveness = 0;
    std::size_t nodes_after_liveness = 0;
    std::size_t speculative_candidates_resolved = 0;
    std::vector<rsb::Candidate> candidates;
};

struct RsbErgReplayOutcome {
    std::vector<RsbErgFrameOutcome> frames;
};

[[nodiscard]] auto replay_rsb_erg_hashes(const capture::CaptureReader& reader,
                                         std::uint64_t seed = 0x5EED1234ULL)
    -> core::Result<RsbErgReplayOutcome>;

[[nodiscard]] auto replay_rsb_erg_hashes(std::span<const std::byte> capture_bytes,
                                         std::uint64_t seed = 0x5EED1234ULL)
    -> core::Result<RsbErgReplayOutcome>;

} // namespace hikari::validate
