#pragma once

#include <cstdint>
#include <span>
#include <vector>

#include "capture/reader.hpp"
#include "core/result.hpp"
#include "validate/validator.hpp"

namespace hikari::runtime {

enum class ExecutionMode : std::uint32_t {
    kSingleThreaded = 1,
    kThreadedSpsc = 2,
};

struct RuntimeConfig {
    ExecutionMode mode = ExecutionMode::kSingleThreaded;
    std::uint64_t seed = 0x5EED1234ULL;
};

struct RuntimeRunReport {
    validate::RunMetrics metrics{};
    std::vector<validate::FrameComparison> comparisons;
    bool reference_available = false;
};

class FrameLoop {
public:
    [[nodiscard]] static auto run_capture(const capture::CaptureReader& reader,
                                          const RuntimeConfig& config)
        -> core::Result<RuntimeRunReport>;

    [[nodiscard]] static auto run_capture(std::span<const std::byte> capture_bytes,
                                          const RuntimeConfig& config)
        -> core::Result<RuntimeRunReport>;
};

} // namespace hikari::runtime
