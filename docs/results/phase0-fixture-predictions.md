# Phase 0 Fixture Predictions (Written Before Implementation and Replay)

This document records the hand-derived predictions for **Fixture B (barrier order)** and **Fixture C (empty frame)** relative to the baseline synthetic fixture (**Fixture A**, `build_synthetic_fixture()`), prior to building or replaying either new fixture.

## Baseline (Fixture A: `build_synthetic_fixture`)

- **Frame 0:**
  1. `BeginFrame(frame_index = 0)`
  2. `ClearOp(target = {1, 1}, rgba = 0xFF0000FF, flags = 0)`
  3. `DrawPatternOp(target = {2, 1}, offset = 0, bytes = [10 20 30 40])`
  4. `EndFrame(frame_index = 0)`
  - Known hash from Phase 0 baseline: `81ddf9f12d69f576db303a4a5e583832`
- **Frame 1:**
  1. `BeginFrame(frame_index = 1)`
  2. `ClearOp(target = {1, 1}, rgba = 0xFF00FF00, flags = 1)`
  3. `DrawPatternOp(target = {2, 1}, offset = 16, bytes = [AA BB CC DD 01 02 03 04])`
  4. `EndFrame(frame_index = 1)`
  - Known hash from Phase 0 baseline: `7b09b468b267821fde6a85f9b4c1e8a5`
- **Frame 2:**
  1. `BeginFrame(frame_index = 2)`
  2. `ClearOp(target = {1, 1}, rgba = 0xFFFF0000, flags = 0)`
  3. `DrawPatternOp(target = {2, 1}, offset = 32, bytes = [DE AD BE EF])`
  4. `DrawPatternOp(target = {1, 1}, offset = 64, bytes = [CA FE BA BE])`
  5. `EndFrame(frame_index = 2)`
  - Known hash from Phase 0 baseline: `759e9e958993af3d594bd8f873755840`

---

## Fixture B: Barrier Order (`build_barrier_reordered_fixture`)

### Operation Sequence

- **Frame 0:** Identical to Fixture A Frame 0.
- **Frame 1:** Identical to Fixture A Frame 1.
- **Frame 2:** In Fixture A, Frame 2 emits `ClearOp`, then `DrawPatternOp` on `vertex_buf` (`{2, 1}`, offset 32, `[DE AD BE EF]`), then `DrawPatternOp` on `color_target` (`{1, 1}`, offset 64, `[CA FE BA BE]`). In Fixture B, a barrier/order shift moves the barrier-Synchronized operation on `color_target` (or reorders the barrier-carrying operation relative to the subsequent read/draw in Frame 2: specifically, moving the barrier flag / operation from before the second draw to after it).

### Predictions for Fixture B

1. **Frame 0 semantic content hash:** **Must match** Fixture A Frame 0 (`81ddf9f12d69f576db303a4a5e583832`), because `replay_semantic_hashes` resets `Xxh3Stream` at each `BeginFrame` and Frame 0's record sequence is unchanged.
2. **Frame 1 semantic content hash:** **Must match** Fixture A Frame 1 (`7b09b468b267821fde6a85f9b4c1e8a5`), because Frame 1's record sequence is unchanged.
3. **Frame 2 semantic content hash:** **Must differ** from Fixture A Frame 2 (`759e9e958993af3d594bd8f873755840`), because `replay_semantic_hashes` feeds each record's `(tag, flags, payload)` into `Xxh3Stream` in stream order; moving the barrier record/order from before the read/draw operation to after it changes the byte sequence fed to XXH3-128 in Frame 2.

---

## Fixture C: Empty Frame (`build_empty_frame_fixture`)

### Operation Sequence

- **Frame 0:** Identical to Fixture A Frame 0 (`BeginFrame(0)`, `ClearOp`, `DrawPatternOp`, `EndFrame(0)`).
- **Frame 1:** Contains **zero operations** between `BeginFrame(1)` and `EndFrame(1)`:
  1. `BeginFrame(frame_index = 1)`
  2. `EndFrame(frame_index = 1)`
- **Frame 2:** Identical to Fixture A Frame 2 (`BeginFrame(2)`, `ClearOp`, `DrawPatternOp`, `DrawPatternOp`, `EndFrame(2)`).

### Predictions for Fixture C

1. **Frame 0 semantic content hash:** **Must match** Fixture A Frame 0 (`81ddf9f12d69f576db303a4a5e583832`), because Frame 0's record sequence is identical.
2. **Frame 1 semantic content hash:** **Must differ** from Fixture A Frame 1 (`7b09b468b267821fde6a85f9b4c1e8a5`), because `Xxh3Stream` in Frame 1 hashes only `BeginFrame(1)` and `EndFrame(1)` (32 bytes total into `Xxh3Stream`) instead of `BeginFrame(1) + ClearOp + DrawPatternOp + EndFrame(1)`.
3. **Frame 2 semantic content hash:** **Must match** Fixture A Frame 2 (`759e9e958993af3d594bd8f873755840`), because `Xxh3Stream` is reset on `BeginFrame(2)` and Frame 2's record sequence is identical to Fixture A Frame 2.
