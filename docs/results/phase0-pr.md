# Phase 0: Capture Format v1 and Deterministic Reference Replay

## Phase

Phase 0 (`AGENTS.md` §8.2 — ground truth).

## Gate result

**PASS** (on synthetic 3-frame captures; hashes are XXH3-128 **semantic content hashes** of the decoded operation stream per frame, **not** rendered-pixel hashes).

Three consecutive runs of `./build/debug/src/validate/hikari_ref_replay /tmp/fixture_a.hkcp`:

```text
=== FIXTURE a (RUN 1) ===
frame=0 semantic_content_hash=81ddf9f12d69f576db303a4a5e583832
frame=1 semantic_content_hash=7b09b468b267821fde6a85f9b4c1e8a5
frame=2 semantic_content_hash=759e9e958993af3d594bd8f873755840
=== FIXTURE a (RUN 2) ===
frame=0 semantic_content_hash=81ddf9f12d69f576db303a4a5e583832
frame=1 semantic_content_hash=7b09b468b267821fde6a85f9b4c1e8a5
frame=2 semantic_content_hash=759e9e958993af3d594bd8f873755840
=== FIXTURE a (RUN 3) ===
frame=0 semantic_content_hash=81ddf9f12d69f576db303a4a5e583832
frame=1 semantic_content_hash=7b09b468b267821fde6a85f9b4c1e8a5
frame=2 semantic_content_hash=759e9e958993af3d594bd8f873755840
=== FIXTURE a DIFF: empty ===
```

## Claims made (and backing tests)

1. **Capture v1 writer → reader round-trip, SHA-256 body integrity check on a flipped byte, and malformed header rejection (`kInvalidMagic`, `kUnsupportedVersion`, `kUnexpectedEof`):** backed by `unit.capture` (`tests/unit/capture_test.cpp`).
2. **Zero calls to global `operator new` inside `replay_semantic_hashes(const CaptureReader&, std::span<FrameSemanticHash>)` after setup:** backed by `unit.replay_alloc` (`tests/unit/replay_alloc_test.cpp`, output: `replay_alloc_test: counted_allocations=0`).
3. **Three consecutive replays of the synthetic fixture produce identical per-frame semantic content hashes:** backed by `golden.determinism` (`tests/golden/determinism_test.cpp`) and the 3-run CLI diff check across Fixtures A, B, and C.
4. **Per-frame semantic content hash changes when operation order changes within a frame (Fixture B Frame 2) or when a frame's operations are removed (Fixture C Frame 1), while unmodified frames retain identical hashes:** backed by `golden.fixture_relations` (`tests/golden/fixture_relations_test.cpp`).
5. **Zero `clang-tidy` findings in repository code across all 12 `.cpp` files under `src/`, `tools/`, and `tests/`:** backed by `docs/results/phase0-clang-tidy.txt` (`Suppressed 128757 warnings (128749 in non-user code, 8 NOLINT)`).

## Claims NOT made

- Does **not** compute rendered-pixel hashes or execute GPU commands on a Vulkan device.
- Does **not** detect cross-frame state carry-over (each frame's `Xxh3Stream` resets at `BeginFrame`).
- Does **not** implement Semantic IR, RSB, ERG, HCT, CPU emulation, loader, or HLE services.
- Does **not** test anything outside the three synthetic fixtures (Fixtures A, B, and C).
- Does **not** pass the `tsan` preset build when `hikari_unit_replay_alloc_test` is linked against ThreadSanitizer's C++ runtime (`libclang_rt.tsan_cxx-x86_64.a` defines its own `operator new`/`operator delete`).

## Commands and exact output

### Debug, Release, ASan, and UBSan CTest runs

```text
$ cmake --preset debug && cmake --build --preset debug && ctest --preset debug
Test project /home/lux/Hikari-Emulator/build/debug
    Start 1: unit.capture
1/4 Test #1: unit.capture .....................   Passed    0.00 sec
    Start 2: unit.replay_alloc
2/4 Test #2: unit.replay_alloc ................   Passed    0.00 sec
    Start 3: golden.determinism
3/4 Test #3: golden.determinism ...............   Passed    0.00 sec
    Start 4: golden.fixture_relations
4/4 Test #4: golden.fixture_relations .........   Passed    0.00 sec

100% tests passed out of 4
```

```text
$ ctest --preset release
100% tests passed out of 4
$ ctest --preset asan
100% tests passed out of 4
$ ctest --preset ubsan
100% tests passed out of 4
```

### TSan preset build failure (exact linker output)

```text
FAILED: [code=1] tests/unit/hikari_unit_replay_alloc_test
/usr/bin/ld: tests/unit/CMakeFiles/hikari_unit_replay_alloc_test.dir/replay_alloc_test.cpp.o: in function `operator new(unsigned long)':
/home/lux/Hikari-Emulator/tests/unit/replay_alloc_test.cpp:27: multiple definition of `operator new(unsigned long)'; /usr/lib/clang/22/lib/linux/libclang_rt.tsan_cxx-x86_64.a(tsan_new_delete.cpp.o):(.text+0x10b0): first defined here
```

### Three-run determinism across Fixtures A, B, and C

```text
=== FIXTURE a (RUN 1..3) ===
frame=0 semantic_content_hash=81ddf9f12d69f576db303a4a5e583832
frame=1 semantic_content_hash=7b09b468b267821fde6a85f9b4c1e8a5
frame=2 semantic_content_hash=759e9e958993af3d594bd8f873755840
=== FIXTURE a DIFF: empty ===
=== FIXTURE b (RUN 1..3) ===
frame=0 semantic_content_hash=81ddf9f12d69f576db303a4a5e583832
frame=1 semantic_content_hash=7b09b468b267821fde6a85f9b4c1e8a5
frame=2 semantic_content_hash=ed17890ecd6316768d29ac52e7b7ffd5
=== FIXTURE b DIFF: empty ===
=== FIXTURE c (RUN 1..3) ===
frame=0 semantic_content_hash=81ddf9f12d69f576db303a4a5e583832
frame=1 semantic_content_hash=88135eb7f2e8891f98e471f16e751e47
frame=2 semantic_content_hash=759e9e958993af3d594bd8f873755840
=== FIXTURE c DIFF: empty ===
```

## Deviations from AGENTS.md

1. **`src/capture/` directory:** Added `src/capture/` for capture format v1 serialization and fixture generation rather than placing capture v1 inside `src/ir/`, keeping `src/ir/` reserved for Semantic IR in later phases.
2. **`.clang-format` `Standard: Latest`:** Documented in `docs/decisions/0002-clang-format-standard.md` because `clang-format` 22.1.8 rejects `Standard: c++23` (`error: unknown enumerated scalar`).

## Known limitations and open questions

- `CaptureReader::open` performs heap allocations inside `picosha2::hash256_one_by_one` during SHA-256 verification at capture open time (setup); replacing PicoSHA2's internal `std::vector` buffer with a fixed 64-byte block buffer would make `CaptureReader::open` allocation-free as well.
- `hikari_unit_replay_alloc_test` cannot link under `-fsanitize=thread` (`tsan` preset) because ThreadSanitizer provides its own strong `operator new`/`operator delete` definitions.
- Capture format questions in `docs/rfcs/0001-capture-format.md` remain open.
