#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "capture/format.hpp"
#include "rsb/rsb.hpp"

namespace hikari::erg {

struct ErgNode {
    rsb::SemanticState state{};
    bool has_side_effect = false;
    bool overwrites_in_full = true;
    bool reads_target = false;
    bool speculative = false;
    std::uint64_t spec_id = 0;
    std::int64_t hotness = 0;
    std::int64_t relevance_score = 0;

    [[nodiscard]] constexpr bool operator==(const ErgNode&) const noexcept = default;
};

class Erg {
public:
    auto add_node(const ErgNode& node) -> rsb::StateId;
    void add_dependency(const rsb::Dependency& dep);

    // 1. Liveness filter (AGENTS.md §7.3):
    // Removes a state ONLY if it is written and overwritten on the same resource
    // before any read (dependency consuming that state), and has_side_effect == false.
    // Returns the number of removed nodes.
    auto run_liveness_pass() -> std::size_t;

    // 2. Hardware bound check hook (AGENTS.md §7.3):
    // Returns true if every node in the graph satisfies hct_is_format_allowed(generation, ...).
    [[nodiscard]] auto check_hardware_bound(capture::GenerationId generation) const noexcept
        -> bool;

    // 3. Relevance ordering (AGENTS.md §7.3, Invariant 3):
    // Updates relevance_score from hotness and returns node indices ordered by
    // descending relevance_score (ties broken by ascending producer_op_index).
    // NEVER removes any node.
    [[nodiscard]] auto compute_relevance_ordering() -> std::vector<std::size_t>;

    [[nodiscard]] auto nodes() const noexcept -> std::span<const ErgNode> {
        return nodes_;
    }

    [[nodiscard]] auto dependencies() const noexcept -> std::span<const rsb::Dependency> {
        return dependencies_;
    }

private:
    std::vector<ErgNode> nodes_;
    std::vector<rsb::Dependency> dependencies_;
};

} // namespace hikari::erg
