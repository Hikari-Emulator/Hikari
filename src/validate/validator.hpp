#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "core/hash.hpp"
#include "core/id.hpp"
#include "core/result.hpp"

namespace hikari::validate {

struct FrameComparison {
    core::FrameIndex frame_index{};
    core::Hash128 main_hash{};
    core::Hash128 reference_hash{};
    bool matched = false;
};

struct RunMetrics {
    std::size_t frames_executed = 0;
    std::size_t frames_matched = 0;
    std::size_t frames_mismatched = 0;
    std::size_t queue_stalls = 0;
    std::size_t pipeline_cache_hits = 0;
    std::size_t pipeline_cache_misses = 0;
};

class Validator {
public:
    [[nodiscard]] static auto compare_frame(core::FrameIndex frame_index, core::Hash128 main_hash,
                                            core::Hash128 reference_hash) noexcept
        -> FrameComparison {
        return FrameComparison{
            .frame_index = frame_index,
            .main_hash = main_hash,
            .reference_hash = reference_hash,
            .matched = (main_hash == reference_hash),
        };
    }
};

} // namespace hikari::validate
