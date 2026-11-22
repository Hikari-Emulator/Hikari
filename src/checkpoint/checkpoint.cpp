#include "checkpoint/checkpoint.hpp"

#include "core/hash.hpp"
#include "core/id.hpp"
#include "core/result.hpp"

namespace hikari::checkpoint {

void CheckpointManager::save_snapshot(core::FrameIndex frame_index, core::Hash128 verified_hash) {
    snapshots_.push_back(Snapshot{
        .frame_index = frame_index,
        .verified_frame_hash = verified_hash,
    });
}

void CheckpointManager::record_override(const OverrideRecord& record) {
    overrides_.push_back(record);
}

auto CheckpointManager::rollback_to_last() noexcept -> core::Result<Snapshot> {
    if (snapshots_.empty()) {
        return core::make_error(core::ErrorCode::kInvalidArgument,
                                "checkpoint::CheckpointManager: no verified snapshot to roll back");
    }
    ++rollback_count_;
    return snapshots_.back();
}

} // namespace hikari::checkpoint
