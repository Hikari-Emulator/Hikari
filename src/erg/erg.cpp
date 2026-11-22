#include "erg/erg.hpp"

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <vector>

#include "capture/format.hpp"
#include "erg/hct_stub.hpp"
#include "rsb/rsb.hpp"

namespace hikari::erg {

namespace {

auto is_state_read(rsb::StateId state_id, std::uint32_t before_op_index,
                   std::span<const rsb::Dependency> dependencies) noexcept -> bool {
    return std::ranges::any_of(dependencies, [&](const rsb::Dependency& dep) {
        return dep.required_state_id == state_id && dep.consumer_op_index <= before_op_index;
    });
}

auto is_overwritten_after(const ErgNode& node, std::span<const ErgNode> all_nodes) noexcept
    -> std::pair<bool, std::uint32_t> {
    bool found = false;
    std::uint32_t next_write_op = 0;
    for (const ErgNode& candidate : all_nodes) {
        if (candidate.state.resource_id == node.state.resource_id &&
            candidate.state.producer_op_index > node.state.producer_op_index) {
            if (!found || candidate.state.producer_op_index < next_write_op) {
                found = true;
                next_write_op = candidate.state.producer_op_index;
            }
        }
    }
    return {found, next_write_op};
}

} // namespace

auto Erg::add_node(const ErgNode& node) -> rsb::StateId {
    nodes_.push_back(node);
    return node.state.state_id;
}

void Erg::add_dependency(const rsb::Dependency& dep) {
    dependencies_.push_back(dep);
}

auto Erg::run_liveness_pass() -> std::size_t {
    std::vector<ErgNode> surviving;
    surviving.reserve(nodes_.size());
    std::size_t removed_count = 0;

    for (const ErgNode& node : nodes_) {
        if (node.has_side_effect) {
            surviving.push_back(node);
            continue;
        }

        const auto [overwritten, next_write_op] = is_overwritten_after(node, nodes_);
        if (overwritten && !is_state_read(node.state.state_id, next_write_op, dependencies_)) {
            ++removed_count;
            continue;
        }

        surviving.push_back(node);
    }

    nodes_ = std::move(surviving);
    return removed_count;
}

auto Erg::check_hardware_bound(capture::GenerationId generation) const noexcept -> bool {
    return std::ranges::all_of(nodes_, [&](const ErgNode& node) {
        return hct_is_format_allowed(generation, node.state.format);
    });
}

auto Erg::compute_relevance_ordering() -> std::vector<std::size_t> {
    for (ErgNode& node : nodes_) {
        node.relevance_score = node.hotness;
    }

    std::vector<std::size_t> order(nodes_.size());
    std::ranges::iota(order, std::size_t{0});

    std::ranges::stable_sort(order, [&](std::size_t lhs, std::size_t rhs) {
        const ErgNode& a = nodes_.at(lhs);
        const ErgNode& b = nodes_.at(rhs);
        if (a.relevance_score != b.relevance_score) {
            return a.relevance_score > b.relevance_score;
        }
        return a.state.producer_op_index < b.state.producer_op_index;
    });

    return order;
}

} // namespace hikari::erg
