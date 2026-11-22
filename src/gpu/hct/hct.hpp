#pragma once

#include <cstdint>
#include <span>
#include <string_view>

#include "capture/format.hpp"
#include "rsb/rsb.hpp"

namespace hikari::gpu::hct {

enum class BoundaryConfidence : std::uint32_t {
    kDocumented = 1,
    kMeasured = 2,
    kInferred = 3,
};

struct BoundarySpec {
    std::uint32_t boundary_id = 0;
    capture::GenerationId generation = capture::GenerationId::kMaxwell;
    BoundaryConfidence confidence = BoundaryConfidence::kDocumented;
    std::string_view source;
    std::string_view verified_by;
};

[[nodiscard]] auto check_boundary_specs(capture::GenerationId generation,
                                        const rsb::SemanticState& state) noexcept -> std::uint32_t;

} // namespace hikari::gpu::hct
