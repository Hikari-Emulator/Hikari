#include "erg/hct_stub.hpp"

#include <algorithm>
#include <array>
#include <span>

#include "capture/format.hpp"
#include "rsb/rsb.hpp"

namespace hikari::erg {

namespace {

// Source: Inferred Maxwell (GM20B) baseline surface format table (confidence = inferred).
constexpr std::array<rsb::ResourceFormat, 4> kMaxwellAllowedFormats = {
    rsb::ResourceFormat::kRgba8Unorm,
    rsb::ResourceFormat::kR32Uint,
    rsb::ResourceFormat::kRgba16Float,
    rsb::ResourceFormat::kD24UnormS8Uint,
};

} // namespace

auto hct_allowed_formats(capture::GenerationId generation) noexcept
    -> std::span<const rsb::ResourceFormat> {
    switch (generation) {
    case capture::GenerationId::kMaxwell:
        return kMaxwellAllowedFormats;
    case capture::GenerationId::kAmpere:
        return {};
    }
    return {};
}

auto hct_is_format_allowed(capture::GenerationId generation, rsb::ResourceFormat format) noexcept
    -> bool {
    const auto allowed = hct_allowed_formats(generation);
    return std::ranges::find(allowed, format) != allowed.end();
}

} // namespace hikari::erg
