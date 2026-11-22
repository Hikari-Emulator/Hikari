#include "rsb/rsb.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "core/hash.hpp"
#include "core/id.hpp"
#include "core/result.hpp"

namespace hikari::rsb {

namespace {

void hash_u32_le(core::Xxh3Stream& stream, std::uint32_t value) noexcept {
    const std::array<std::byte, 4> bytes = {
        static_cast<std::byte>(value & 0xFFU),
        static_cast<std::byte>((value >> 8U) & 0xFFU),
        static_cast<std::byte>((value >> 16U) & 0xFFU),
        static_cast<std::byte>((value >> 24U) & 0xFFU),
    };
    stream.update(bytes);
}

void hash_u64_le(core::Xxh3Stream& stream, std::uint64_t value) noexcept {
    std::array<std::byte, 8> bytes{};
    unsigned shift = 0;
    for (std::byte& b : bytes) {
        b = static_cast<std::byte>((value >> shift) & 0xFFU);
        shift += 8U;
    }
    stream.update(bytes);
}

auto contains_format(std::span<const ResourceFormat> list, ResourceFormat fmt) noexcept -> bool {
    return std::ranges::find(list, fmt) != list.end();
}

} // namespace

auto compute_semantic_state_hash(core::ResourceId resource_id, ResourceFormat format,
                                 Dimensions dimensions, std::uint32_t binding_flags,
                                 std::uint32_t producer_op_index) noexcept -> core::Hash128 {
    core::Xxh3Stream stream;
    hash_u32_le(stream, resource_id.index);
    hash_u32_le(stream, resource_id.generation);
    hash_u32_le(stream, static_cast<std::uint32_t>(format));
    hash_u32_le(stream, dimensions.width);
    hash_u32_le(stream, dimensions.height);
    hash_u32_le(stream, dimensions.depth);
    hash_u32_le(stream, binding_flags);
    hash_u32_le(stream, producer_op_index);
    return stream.finalize();
}

auto Rsb::resolve(const Dependency& missing_dependency, const ResolveContext& context) noexcept
    -> core::Result<Candidate> {
    if (context.capture_known_formats.empty()) {
        return core::make_error(core::ErrorCode::kInvalidArgument,
                                "RSB candidate set is empty (no capture-known formats)");
    }
    if (context.hct_allowed_formats.empty()) {
        return core::make_error(core::ErrorCode::kInvalidArgument,
                                "RSB HCT allowed format set is empty");
    }

    // Build the bounded permitted candidate format list on the stack (max 16 formats).
    // If any format in capture_known_formats is outside hct_allowed_formats, it is filtered out;
    // if no permitted formats remain, RSB rejects resolution.
    constexpr std::size_t kMaxCandidateFormats = 16;
    std::array<ResourceFormat, kMaxCandidateFormats> permitted{};
    std::size_t permitted_count = 0;

    for (const ResourceFormat candidate_fmt : context.capture_known_formats) {
        if (!contains_format(context.hct_allowed_formats, candidate_fmt)) {
            continue;
        }
        const std::span<const ResourceFormat> current_permitted =
            std::span<const ResourceFormat>{permitted}.subspan(0, permitted_count);
        if (contains_format(current_permitted, candidate_fmt)) {
            continue;
        }
        if (permitted_count >= kMaxCandidateFormats) {
            return core::make_error(core::ErrorCode::kOverflow,
                                    "RSB permitted candidate set exceeded fixed capacity");
        }
        permitted.at(permitted_count++) = candidate_fmt;
    }

    if (permitted_count == 0U) {
        return core::make_error(core::ErrorCode::kInvalidArgument,
                                "RSB rejected all candidates due to HCT bound violation");
    }

    // Deterministic selection from the permitted candidate set given (seed, dependency).
    core::Xxh3Stream selector;
    hash_u64_le(selector, context.seed);
    hash_u32_le(selector, missing_dependency.consumer_op_index);
    hash_u32_le(selector, missing_dependency.required_state_id.index);
    hash_u32_le(selector, missing_dependency.required_state_id.generation);
    hash_u32_le(selector, missing_dependency.required_resource_id.index);
    hash_u32_le(selector, missing_dependency.required_resource_id.generation);
    const core::Hash128 selection_hash = selector.finalize();

    const auto chosen_idx =
        static_cast<std::size_t>(selection_hash.low % static_cast<std::uint64_t>(permitted_count));
    const ResourceFormat chosen_format = permitted.at(chosen_idx);

    const SemanticState state{
        .state_id = missing_dependency.required_state_id,
        .resource_id = missing_dependency.required_resource_id,
        .format = chosen_format,
        .dimensions = context.default_dimensions,
        .binding_flags = context.default_binding_flags,
        .content_hash = compute_semantic_state_hash(missing_dependency.required_resource_id,
                                                    chosen_format, context.default_dimensions,
                                                    context.default_binding_flags, 0U),
        .producer_op_index = 0U,
    };

    return Candidate{
        .state = state,
        .speculative = true,
        .spec_id = context.next_spec_id,
        .guard =
            Guard{
                .resource_id = missing_dependency.required_resource_id,
                .expected_version = context.assumed_resource_version,
            },
    };
}

} // namespace hikari::rsb
