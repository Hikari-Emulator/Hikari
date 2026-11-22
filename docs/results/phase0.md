# Phase 0 Results: Capture Format v1 and Deterministic Reference Replay

- **Gate (`AGENTS.md` §8.2):** "Replay of one deterministic capture produces identical hashes across 3 runs."
- **Status:** Pass (on synthetic 3-frame capture; semantic content hashes only, **not** rendered-pixel hashes).

## Verification Runs

### Three Consecutive CLI Replays

Commands:

```bash
./build/debug/tools/capdump/hikari_capdump --write-synthetic /tmp/synthetic_v1.hkcp
./build/debug/src/validate/hikari_ref_replay /tmp/synthetic_v1.hkcp > /tmp/replay_run1.txt
./build/debug/src/validate/hikari_ref_replay /tmp/synthetic_v1.hkcp > /tmp/replay_run2.txt
./build/debug/src/validate/hikari_ref_replay /tmp/synthetic_v1.hkcp > /tmp/replay_run3.txt
diff -u /tmp/replay_run1.txt /tmp/replay_run2.txt
diff -u /tmp/replay_run1.txt /tmp/replay_run3.txt
```

Output (identical across runs 1, 2, and 3; `diff -u` produced empty output):

```text
frame=0 semantic_content_hash=81ddf9f12d69f576db303a4a5e583832
frame=1 semantic_content_hash=7b09b468b267821fde6a85f9b4c1e8a5
frame=2 semantic_content_hash=759e9e958993af3d594bd8f873755840
```

### CTest Presets

- `debug`: `100% tests passed, 0 tests failed out of 2` (`unit.capture`, `golden.determinism`)
- `asan`: `100% tests passed, 0 tests failed out of 2` (`unit.capture`, `golden.determinism`)
- `ubsan`: `100% tests passed, 0 tests failed out of 2` (`unit.capture`, `golden.determinism`)

## Scope and Limitations

- Output hashes are XXH3-128 **semantic content hashes** of the decoded capture operation stream per frame, not rendered-pixel hashes.
- Tested only on a 3-frame synthetic fixture (`build_synthetic_fixture()`), not on hardware captures or games.
- No Vulkan device, Semantic IR, RSB, ERG, HCT, CPU, loader, or HLE services are implemented in Phase 0.
