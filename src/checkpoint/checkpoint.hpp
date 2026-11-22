#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "core/hash.hpp"
#include "core/id.hpp"
#include "core/result.hpp"
#include "rsb/rsb.hpp"

namespace hikari::checkpoint {

struct OverrideRecord {
    std::uint64_t spec_id = 0;
    rsb::StateId state_id{};
    core::FrameIndex frame_index_at_quarantine{};
    core::FrameIndex frame_index_at_override{};
    core::Hash128 hash_before{};
    core::Hash128 hash_after{};
};

struct Snapshot {
    core::FrameIndex frame_index{};
    core::Hash128 verified_frame_hash{};
};

class CheckpointManager {
public:
    void save_snapshot(core::FrameIndex frame_index, core::Hash128 verified_hash);
    void record_override(const OverrideRecord& record);

    [[nodiscard]] auto rollback_to_last() noexcept -> core::Result<Snapshot>;

    [[nodiscard]] auto snapshots() const noexcept -> std::span<const Snapshot> {
        return snapshots_;
    }

    [[nodiscard]] auto overrides() const noexcept -> std::span<const OverrideRecord> {
        return overrides_;
    }

    [[nodiscard]] auto rollback_count() const noexcept -> std::size_t {
        return rollback_count_;
    }

private:
    std::vector<Snapshot> snapshots_;
    std::vector<OverrideRecord> overrides_;
    std::size_t rollback_count_ = 0;
};

} // namespace hikari::checkpoint
