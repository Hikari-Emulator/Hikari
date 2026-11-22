# RSB and ERG Predictions for Fixtures D and E (Written Before Implementation and Replay)

This document records the hand-derived predictions for **Fixture D (incomplete dependency)** and **Fixture E (dead write)** before building or replaying either fixture.

---

## Fixture D: Incomplete Dependency (`build_incomplete_dependency_fixture`)

### Capture Operation Sequence

Single frame (`frame_index = 0`):
1. `BeginFrame(frame_index = 0)`
2. `ClearOp(target = {1, 1}, rgba = 0xFF0000FF, flags = 0)` — binds/writes resource `{1, 1}` (`color_target`).
3. `SampleOp(target = {1, 1}, sampled = {3, 1}, flags = 0)` — draws into `{1, 1}` while sampling resource `R = {3, 1}`, which was **never written or bound earlier in the capture**.
4. `EndFrame(frame_index = 0)`

### Expected Reference Path Behavior on Fixture D

- **Reference path policy for unresolved reads:** The reference replay (`replay_semantic_hashes`) tracks resources written in the capture and **never guesses**. When it encounters `SampleOp` sampling `R = {3, 1}` with no prior producer write to `{3, 1}`, it must reject the capture and return a reported error:
  - `ErrorCode::kInvalidArgument` (`"unresolved resource dependency in reference replay"`).
- Therefore, on Fixture D directly, the reference path returns an error rather than a frame hash.

### Expected RSB + ERG Behavior on Fixture D

- When the RSB+ERG path encounters `SampleOp(target = {1, 1}, sampled = {3, 1}, flags = 0)` at consumer op index `2` with no prior state for `R = {3, 1}`:
  - It constructs `missing_dependency = Dependency{consumer_op_index = 2, required_state_id = {index = 2, generation = 1}, required_resource_id = {index = 3, generation = 1}}`.
  - It invokes `Rsb::resolve(missing_dependency, context)` where `context.capture_known_formats = {ResourceFormat::kRgba8Unorm}` (the format known from the capture's `ClearOp`) and `context.hct_allowed_formats = hct_allowed_formats(GenerationId::kMaxwell)`.
  - **Predicted RSB Candidate:**
    - `speculative = true`
    - `spec_id = 1`
    - `guard = Guard{resource_id = {3, 1}, expected_version = 0}`
    - `state.state_id = {2, 1}`
    - `state.resource_id = {3, 1}`
    - `state.format = ResourceFormat::kRgba8Unorm` (the sole element in `capture_known_formats ∩ hct_allowed_formats`)
    - `state.dimensions = {64, 64, 1}`
    - `state.binding_flags = 1`
    - `state.producer_op_index = 0`
- **Definition of "correct" for Fixture D Frame 0 hash:**
  - If a capture explicitly initializes resource `R = {3, 1}` before `SampleOp` with the exact state that RSB hypothesizes (or when the RSB+ERG path resolves the missing dependency for `R = {3, 1}` and emits the surviving frame operations `BeginFrame(0) -> ClearOp({1, 1}) -> SampleOp({1, 1}, {3, 1}) -> EndFrame(0)`), the Frame 0 semantic content hash of the RSB+ERG path must equal the XXH3-128 hash of that 4-record stream (`BeginFrame(0)`, `ClearOp({1, 1}, 0xFF0000FF, 0)`, `SampleOp({1, 1}, {3, 1}, 0)`, `EndFrame(0)`).
  - Specifically, on Fixture D itself:
    - **Reference path:** returns error `ErrorCode::kInvalidArgument` (unresolved resource `{3, 1}`).
    - **RSB+ERG path:** succeeds (1 frame, 1 speculative candidate resolved with `spec_id = 1`, `format = kRgba8Unorm`, `guard = {{3, 1}, 0}`, 0 nodes pruned by liveness).

---

## Fixture E: Dead Write (`build_dead_write_fixture`)

### Capture Operation Sequence

Three frames (`frame_index = 0, 1, 2`), identical to **Fixture A (`build_synthetic_fixture`)** except Frame 0 has an extra `ClearOp` on `color_target` (`{1, 1}`) that is immediately overwritten by the original `ClearOp` before any read of `{1, 1}`:

- **Frame 0:**
  1. `BeginFrame(frame_index = 0)`
  2. **Extra dead write:** `ClearOp(target = {1, 1}, rgba = 0x11223344, flags = 0)` (producer op 1, `has_side_effect = false`)
  3. **Overwriting write:** `ClearOp(target = {1, 1}, rgba = 0xFF0000FF, flags = 0)` (producer op 2, overwrites `{1, 1}` before any read)
  4. `DrawPatternOp(target = {2, 1}, offset = 0, bytes = [10 20 30 40])`
  5. `EndFrame(frame_index = 0)`
- **Frame 1:** Identical to Fixture A Frame 1.
- **Frame 2:** Identical to Fixture A Frame 2.

### Predictions for Fixture E

1. **ERG Liveness Pass on Fixture E:**
   - Must remove **exactly 1 node** (the dead write `ClearOp(target = {1, 1}, rgba = 0x11223344, flags = 0)` in Frame 0), and **no other node**.
   - After ERG liveness pruning removes the dead write in Frame 0, the surviving operation stream of Fixture E in the RSB+ERG path is identical to **Fixture A**:
     - Frame 0 hash of RSB+ERG on Fixture E **must equal** Fixture A Frame 0 (`81ddf9f12d69f576db303a4a5e583832`).
     - Frame 1 hash of RSB+ERG on Fixture E **must equal** Fixture A Frame 1 (`7b09b468b267821fde6a85f9b4c1e8a5`).
     - Frame 2 hash of RSB+ERG on Fixture E **must equal** Fixture A Frame 2 (`759e9e958993af3d594bd8f873755840`).
2. **Differential Comparison (RSB+ERG path vs. unpruned Reference Replay on Fixture E):**
   - Because the non-speculative reference replay (`replay_semantic_hashes`) hashes every record present in the capture stream without ERG liveness pruning (`AGENTS.md` §0: "never route it through RSB/ERG/caches"), the reference replay on Fixture E hashes the unpruned dead write in Frame 0, whereas the RSB+ERG path removes the dead write in Frame 0:
     - **Fixture E Frame 0:** Reference replay on Fixture E includes the extra dead write record, while RSB+ERG on Fixture E prunes it (matching Fixture A Frame 0 `81ddf9f12d69f576db303a4a5e583832`). Thus, direct stream hash comparison between unpruned Reference(E) and pruned RSB+ERG(E) on Frame 0 will **differ** (while RSB+ERG(E) Frame 0 **matches** Reference(A) Frame 0!).
     - **Fixture E Frame 1 and Frame 2:** RSB+ERG(E) and Reference(E) **must match** (`7b09b468b267821fde6a85f9b4c1e8a5` and `759e9e958993af3d594bd8f873755840`).
