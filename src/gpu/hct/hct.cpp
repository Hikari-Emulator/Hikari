#include "gpu/hct/hct.hpp"

#include <cstdint>

#include "capture/format.hpp"
#include "erg/hct_stub.hpp"
#include "rsb/rsb.hpp"

namespace hikari::gpu::hct {

auto check_boundary_specs(capture::GenerationId generation,
                          const rsb::SemanticState& state) noexcept -> std::uint32_t {
    // Returns 0 if all boundary predicates hold; otherwise returns the violated boundary_id.
    if (!erg::hct_is_format_allowed(generation, state.format)) {
        return 1001U;
    }
    if (state.dimensions.width > 16384U || state.dimensions.height > 16384U) {
        return 1002U;
    }
    return 0U;
}

} // namespace hikari::gpu::hct
