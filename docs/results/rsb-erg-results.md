# RSB and ERG Experimental Results

## Overview

This document records the measured outputs of the experimental Recursive Semantic Bootstrapper (`src/rsb/`) and Execution Relevance Graph (`src/erg/`) modules tested against the Phase 0 reference replay (`src/validate/reference_replay.cpp`) across Fixtures A, B, C, D, and E.

---

## Differential Table (`golden.rsb_erg_diff`)

Verbatim output of `./build/debug/tests/golden/hikari_golden_rsb_erg_diff_test`:

```text
diff fixture=A_synthetic frame=0 reference=81ddf9f12d69f576db303a4a5e583832 rsb_erg=81ddf9f12d69f576db303a4a5e583832 status=MATCH spec_candidates=0 pruned=0
diff fixture=A_synthetic frame=1 reference=7b09b468b267821fde6a85f9b4c1e8a5 rsb_erg=7b09b468b267821fde6a85f9b4c1e8a5 status=MATCH spec_candidates=0 pruned=0
diff fixture=A_synthetic frame=2 reference=759e9e958993af3d594bd8f873755840 rsb_erg=759e9e958993af3d594bd8f873755840 status=MATCH spec_candidates=0 pruned=0
diff fixture=B_barrier_reordered frame=0 reference=81ddf9f12d69f576db303a4a5e583832 rsb_erg=81ddf9f12d69f576db303a4a5e583832 status=MATCH spec_candidates=0 pruned=0
diff fixture=B_barrier_reordered frame=1 reference=7b09b468b267821fde6a85f9b4c1e8a5 rsb_erg=7b09b468b267821fde6a85f9b4c1e8a5 status=MATCH spec_candidates=0 pruned=0
diff fixture=B_barrier_reordered frame=2 reference=ed17890ecd6316768d29ac52e7b7ffd5 rsb_erg=ed17890ecd6316768d29ac52e7b7ffd5 status=MATCH spec_candidates=0 pruned=0
diff fixture=C_empty_frame frame=0 reference=81ddf9f12d69f576db303a4a5e583832 rsb_erg=81ddf9f12d69f576db303a4a5e583832 status=MATCH spec_candidates=0 pruned=0
diff fixture=C_empty_frame frame=1 reference=88135eb7f2e8891f98e471f16e751e47 rsb_erg=88135eb7f2e8891f98e471f16e751e47 status=MATCH spec_candidates=0 pruned=0
diff fixture=C_empty_frame frame=2 reference=759e9e958993af3d594bd8f873755840 rsb_erg=759e9e958993af3d594bd8f873755840 status=MATCH spec_candidates=0 pruned=0
diff fixture=D_incomplete_dep frame=0 reference=ERROR(code=1) rsb_erg=dedd2fb55e2c820eb4e96ad80f15c1e5 status=REF_ERROR_RSB_RESOLVED spec_candidates=1 pruned=1
diff fixture=E_dead_write frame=0 reference=d85962f567646e4ca318e751b4068a37 rsb_erg=81ddf9f12d69f576db303a4a5e583832 status=MISMATCH spec_candidates=0 pruned=1
diff fixture=E_dead_write frame=1 reference=7b09b468b267821fde6a85f9b4c1e8a5 rsb_erg=7b09b468b267821fde6a85f9b4c1e8a5 status=MATCH spec_candidates=0 pruned=0
diff fixture=E_dead_write frame=2 reference=759e9e958993af3d594bd8f873755840 rsb_erg=759e9e958993af3d594bd8f873755840 status=MATCH spec_candidates=0 pruned=0
```

| Fixture | Frame | Reference Hash / Outcome | RSB+ERG Hash | Status | Speculative Candidates | Liveness Pruned |
| --- | --- | --- | --- | --- | --- | --- |
| `A_synthetic` | 0 | `81ddf9f12d69f576db303a4a5e583832` | `81ddf9f12d69f576db303a4a5e583832` | `MATCH` | 0 | 0 |
| `A_synthetic` | 1 | `7b09b468b267821fde6a85f9b4c1e8a5` | `7b09b468b267821fde6a85f9b4c1e8a5` | `MATCH` | 0 | 0 |
| `A_synthetic` | 2 | `759e9e958993af3d594bd8f873755840` | `759e9e958993af3d594bd8f873755840` | `MATCH` | 0 | 0 |
| `B_barrier_reordered` | 0 | `81ddf9f12d69f576db303a4a5e583832` | `81ddf9f12d69f576db303a4a5e583832` | `MATCH` | 0 | 0 |
| `B_barrier_reordered` | 1 | `7b09b468b267821fde6a85f9b4c1e8a5` | `7b09b468b267821fde6a85f9b4c1e8a5` | `MATCH` | 0 | 0 |
| `B_barrier_reordered` | 2 | `ed17890ecd6316768d29ac52e7b7ffd5` | `ed17890ecd6316768d29ac52e7b7ffd5` | `MATCH` | 0 | 0 |
| `C_empty_frame` | 0 | `81ddf9f12d69f576db303a4a5e583832` | `81ddf9f12d69f576db303a4a5e583832` | `MATCH` | 0 | 0 |
| `C_empty_frame` | 1 | `88135eb7f2e8891f98e471f16e751e47` | `88135eb7f2e8891f98e471f16e751e47` | `MATCH` | 0 | 0 |
| `C_empty_frame` | 2 | `759e9e958993af3d594bd8f873755840` | `759e9e958993af3d594bd8f873755840` | `MATCH` | 0 | 0 |
| `D_incomplete_dep` | 0 | `ERROR(code=1)` (`kInvalidArgument`) | `dedd2fb55e2c820eb4e96ad80f15c1e5` | `REF_ERROR_RSB_RESOLVED` | 1 | 1 |
| `E_dead_write` | 0 | `d85962f567646e4ca318e751b4068a37` | `81ddf9f12d69f576db303a4a5e583832` | `MISMATCH` (matches `A` Frame 0) | 0 | 1 |
| `E_dead_write` | 1 | `7b09b468b267821fde6a85f9b4c1e8a5` | `7b09b468b267821fde6a85f9b4c1e8a5` | `MATCH` | 0 | 0 |
| `E_dead_write` | 2 | `759e9e958993af3d594bd8f873755840` | `759e9e958993af3d594bd8f873755840` | `MATCH` | 0 | 0 |

---

## Predictions vs. Actual Results (Fixtures D and E)

1. **Fixture D (`build_incomplete_dependency_fixture`):**
   - **Matched predictions:**
     - Reference path rejected the unresolved read of `{3, 1}` with `ErrorCode::kInvalidArgument` (`code=1`).
     - RSB resolved 1 speculative candidate with `spec_id = 1`, `resource_id = {3, 1}`, `format = kRgba8Unorm`, `guard = {{3, 1}, 0}`.
   - **Prediction that was wrong (and why):**
     - `docs/results/rsb-erg-predictions.md` predicted `0 nodes pruned by liveness` on Fixture D, and that the surviving stream would be `BeginFrame(0) -> ClearOp({1, 1}) -> SampleOp({1, 1}, {3, 1}) -> EndFrame(0)`.
     - **Actual result:** `pruned=1` on Fixture D! Why: in `build_incomplete_dependency_fixture()`, `ClearOp` writes `color_target = {1, 1}` (op 1, `has_side_effect = false`), and then `SampleOp` writes `target = {1, 1}` while only *sampling* `sampled = {3, 1}` (op 2). Because `SampleOp` overwrites `{1, 1}` without adding a read dependency on `{1, 1}`'s prior state, ERG's liveness pass proved `ClearOp({1, 1})` dead and pruned it (`pruned=1`), leaving `BeginFrame(0) -> SampleOp({1, 1}, {3, 1}) -> EndFrame(0)` (`dedd2fb55e2c820eb4e96ad80f15c1e5`).

2. **Fixture E (`build_dead_write_fixture`):**
   - **All predictions for Fixture E matched:**
     - ERG liveness removed `1` node in Frame 0 (the dead `ClearOp({1, 1}, 0x11223344, 0)`) and `0` nodes in Frames 1 and 2 (`frame0_before=3 frame0_removed=1 frame0_after=2 frame1_removed=0 frame2_removed=0`).
     - After pruning the dead write in Frame 0, RSB+ERG on Fixture E produced `81ddf9f12d69f576db303a4a5e583832` for Frame 0, `7b09b468b267821fde6a85f9b4c1e8a5` for Frame 1, and `759e9e958993af3d594bd8f873755840` for Frame 2 — matching Fixture A (`build_synthetic_fixture`) frame by frame.
     - Comparing unpruned Reference(E) directly against pruned RSB+ERG(E) showed `MISMATCH` on Frame 0 (`d85962f567646e4ca318e751b4068a37` vs `81ddf9f12d69f576db303a4a5e583832`) because the reference path hashes the unpruned capture stream without ERG pruning.

---

## Unverified Assumptions and Limitations

- **HCT stub (`src/erg/hct_stub.cpp`):** The allowed format list for Maxwell (`kRgba8Unorm`, `kR32Uint`, `kRgba16Float`, `kD24UnormS8Uint`) is marked `confidence = inferred` and is not verified against hardware or generated from `data/hct/*.toml`.
- **Stream hash vs. rendered-pixel hash:** Because hashes are computed over the surviving operation stream rather than rendered pixels, pruning a dead write changes the stream hash relative to an unpruned replay of the same file (Fixture E Frame 0 vs. Reference(E) Frame 0), and matches the stream hash of the baseline capture without that dead write (Reference(A) Frame 0).
- **Partial write modeling:** `DrawPatternOp` is modeled in `rsb_erg_replay.cpp` as depending on the prior state of its target resource so that partial/subrange writes (or clear followed by subrange draw in Frame 2) do not falsely prune earlier full-resource clears.
