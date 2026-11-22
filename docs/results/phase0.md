# Phase 0 Results: Capture Format v1 and Deterministic Reference Replay

- **Gate (`AGENTS.md` §8.2):** "Replay of one deterministic capture produces identical hashes across 3 runs."
- **Status:** Pass (on synthetic 3-frame captures A, B, and C; semantic content hashes only, **not** rendered-pixel hashes).

## Verification Runs

### Three Consecutive CLI Replays Across All Three Fixtures

Commands:

```bash
./build/debug/tools/capdump/hikari_capdump --write-synthetic /tmp/fixture_a.hkcp
./build/debug/tools/capdump/hikari_capdump --write-barrier-reordered /tmp/fixture_b.hkcp
./build/debug/tools/capdump/hikari_capdump --write-empty-frame /tmp/fixture_c.hkcp
for f in a b c; do
  for r in 1 2 3; do
    ./build/debug/src/validate/hikari_ref_replay /tmp/fixture_${f}.hkcp > /tmp/fixture_${f}_run${r}.txt
  done
  diff -u /tmp/fixture_${f}_run1.txt /tmp/fixture_${f}_run2.txt
  diff -u /tmp/fixture_${f}_run1.txt /tmp/fixture_${f}_run3.txt
done
```

#### Fixture A (`build_synthetic_fixture`, 452 bytes)

Runs 1, 2, and 3 (`diff -u` output empty):

```text
frame=0 semantic_content_hash=81ddf9f12d69f576db303a4a5e583832
frame=1 semantic_content_hash=7b09b468b267821fde6a85f9b4c1e8a5
frame=2 semantic_content_hash=759e9e958993af3d594bd8f873755840
```

#### Fixture B (`build_barrier_reordered_fixture`, 452 bytes)

Runs 1, 2, and 3 (`diff -u` output empty):

```text
frame=0 semantic_content_hash=81ddf9f12d69f576db303a4a5e583832
frame=1 semantic_content_hash=7b09b468b267821fde6a85f9b4c1e8a5
frame=2 semantic_content_hash=ed17890ecd6316768d29ac52e7b7ffd5
```

Comparison with predictions (`docs/results/phase0-fixture-predictions.md`):
- Frame 0 matches Fixture A Frame 0 (`81ddf9f12d69f576db303a4a5e583832`).
- Frame 1 matches Fixture A Frame 1 (`7b09b468b267821fde6a85f9b4c1e8a5`).
- Frame 2 differs from Fixture A Frame 2 (`ed17890ecd6316768d29ac52e7b7ffd5` vs `759e9e958993af3d594bd8f873755840`).
- Zero predictions were wrong.

#### Fixture C (`build_empty_frame_fixture`, 380 bytes)

Runs 1, 2, and 3 (`diff -u` output empty):

```text
frame=0 semantic_content_hash=81ddf9f12d69f576db303a4a5e583832
frame=1 semantic_content_hash=88135eb7f2e8891f98e471f16e751e47
frame=2 semantic_content_hash=759e9e958993af3d594bd8f873755840
```

Comparison with predictions (`docs/results/phase0-fixture-predictions.md`):
- Frame 0 matches Fixture A Frame 0 (`81ddf9f12d69f576db303a4a5e583832`).
- Frame 1 differs from Fixture A Frame 1 (`88135eb7f2e8891f98e471f16e751e47` vs `7b09b468b267821fde6a85f9b4c1e8a5`).
- Frame 2 matches Fixture A Frame 2 (`759e9e958993af3d594bd8f873755840`).
- Zero predictions were wrong.

### Replay Loop Allocation Check (`unit.replay_alloc`)

- Global `operator new` / `operator delete` counter armed after setup (`build_synthetic_fixture`, `CaptureReader::open`, and stack output buffer allocation) and disarmed immediately after `replay_semantic_hashes(const CaptureReader&, std::span<FrameSemanticHash>)`:
  - `replay_alloc_test: counted_allocations=0`
- Note: `CaptureReader::open` computes a SHA-256 integrity check over the capture body via `picosha2::hash256_one_by_one`, which allocates a `std::vector` buffer internally during setup (`CaptureReader::open`), before the replay loop begins.

### CTest Presets

- `debug`: `100% tests passed out of 4` (`unit.capture`, `unit.replay_alloc`, `golden.determinism`, `golden.fixture_relations`)
- `release`: `100% tests passed out of 4`
- `asan`: `100% tests passed out of 4`
- `ubsan`: `100% tests passed out of 4`
- `tsan`: Build failed while linking `hikari_unit_replay_alloc_test` because ThreadSanitizer's runtime (`libclang_rt.tsan_cxx-x86_64.a(tsan_new_delete.cpp.o)`) defines strong symbols for `operator new` and `operator delete`, conflicting with `replay_alloc_test.cpp`'s global `operator new` override (`multiple definition of operator new(unsigned long)`). All other targets (`hikari_unit_capture_test`, `hikari_golden_determinism_test`, `hikari_ref_replay`) linked under `tsan`. `grep -rn "std::thread\|std::jthread\|std::async" src tools tests` returned zero matches.

## What the hash does not detect

1. **Changes that do not alter the per-frame record byte stream** (and conversely, the hash treats any reordering of independent operations in the stream as a difference because it hashes serialized `(tag, flags, payload)` records in stream order without a commutative semantic equivalence model).
2. **Capture header metadata outside the record stream** (for example, `generation_id` or `reserved` header fields are validated by `CaptureReader::open` and covered by the file structure, but are not fed into the per-frame `Xxh3Stream`).
3. **Cross-frame state carry-over** (because `Xxh3Stream` resets at each `BeginFrame`; as shown by Fixture C Frame 2 matching Fixture A Frame 2 (`759e9e958993af3d594bd8f873755840`) even though Frame 1's operations were removed in Fixture C).
4. **Rendered output.** The hash does not execute GPU operations or observe pixels.
5. **Anything outside the three synthetic fixtures** (Fixtures A, B, and C).
