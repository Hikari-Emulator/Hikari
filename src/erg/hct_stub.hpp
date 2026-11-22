#pragma once

#include <span>

#include "capture/format.hpp"
#include "rsb/rsb.hpp"

namespace hikari::erg {

// HCT stub (AGENTS.md §7.3, §11):
// Source: Inferred baseline subset of Maxwell (GM20B) color/buffer/depth surface formats
// (confidence = inferred; not verified against hardware or conformance tests yet).
// Used only as the exploratory bound-check hook for RSB candidate filtering.
[[nodiscard]] auto hct_allowed_formats(capture::GenerationId generation) noexcept
    -> std::span<const rsb::ResourceFormat>;

[[nodiscard]] auto hct_is_format_allowed(capture::GenerationId generation,
                                         rsb::ResourceFormat format) noexcept -> bool;

} // namespace hikari::erg
