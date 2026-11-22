#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "core/hash.hpp"
#include "core/id.hpp"
#include "core/result.hpp"

namespace hikari::rsb {

struct StateId {
    std::uint32_t index = 0;
    std::uint32_t generation = 0;

    [[nodiscard]] constexpr bool operator==(const StateId&) const noexcept = default;
};

static_assert(sizeof(StateId) == 8);

enum class ResourceFormat : std::uint32_t {
    kUnknown = 0,
    kRgba8Unorm = 1,
    kR32Uint = 2,
    kRgba16Float = 3,
    kD24UnormS8Uint = 4,
    // Intentionally unsupported on Maxwell in the HCT stub (used for bound rejection tests).
    kInvalidExperimentalBc7 = 999,
};

struct Dimensions {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t depth = 1;

    [[nodiscard]] constexpr bool operator==(const Dimensions&) const noexcept = default;
};

struct SemanticState {
    StateId state_id{};
    core::ResourceId resource_id{};
    ResourceFormat format = ResourceFormat::kRgba8Unorm;
    Dimensions dimensions{};
    std::uint32_t binding_flags = 0;
    core::Hash128 content_hash{};
    std::uint32_t producer_op_index = 0;

    [[nodiscard]] constexpr bool operator==(const SemanticState&) const noexcept = default;
};

[[nodiscard]] auto compute_semantic_state_hash(core::ResourceId resource_id, ResourceFormat format,
                                               Dimensions dimensions, std::uint32_t binding_flags,
                                               std::uint32_t producer_op_index) noexcept
    -> core::Hash128;

struct Dependency {
    std::uint32_t consumer_op_index = 0;
    StateId required_state_id{};
    core::ResourceId required_resource_id{};

    [[nodiscard]] constexpr bool operator==(const Dependency&) const noexcept = default;
};

struct Guard {
    core::ResourceId resource_id{};
    std::uint32_t expected_version = 0;

    [[nodiscard]] constexpr bool operator==(const Guard&) const noexcept = default;
};

struct Candidate {
    SemanticState state{};
    bool speculative = true;
    std::uint64_t spec_id = 0;
    Guard guard{};

    [[nodiscard]] constexpr bool operator==(const Candidate&) const noexcept = default;
};

struct ResolveContext {
    std::uint64_t seed = 0;
    std::uint64_t next_spec_id = 1;
    std::uint32_t assumed_resource_version = 0;
    Dimensions default_dimensions{.width = 64, .height = 64, .depth = 1};
    std::uint32_t default_binding_flags = 1;
    std::span<const ResourceFormat> capture_known_formats;
    std::span<const ResourceFormat> hct_allowed_formats;
};

class Rsb {
public:
    // Deterministically selects a candidate from the intersection of
    // context.capture_known_formats and context.hct_allowed_formats.
    // Never invents a format outside that set.
    [[nodiscard]] static auto resolve(const Dependency& missing_dependency,
                                      const ResolveContext& context) noexcept
        -> core::Result<Candidate>;
};

} // namespace hikari::rsb
