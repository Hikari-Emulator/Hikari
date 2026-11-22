# Hikari

Hikari is a research project on bounded speculative GPU state reconstruction for Nintendo Switch emulation. Guest CPU code runs on Dynarmic. Guest GPU commands are decoded into a versioned Semantic IR, tracked by an Execution Relevance Graph (ERG), and executed by a Vulkan executor that makes no semantic decisions.

## Status

Early-stage research prototype. **Hikari cannot run games.** The current repository contains a scaffold that runs synthetic captures and a Dynarmic integration under test. Every architectural and performance claim here is a hypothesis until a reproducible test in this repository demonstrates it. Measured results, including negative ones, are recorded in `docs/results/`.

## Runtime model

The authoritative description is [`AGENTS.md`](AGENTS.md) §7 and [`docs/rfcs/0002-state-lifecycle.md`](docs/rfcs/0002-state-lifecycle.md). In summary:

1. **Guest CPU** (Dynarmic, A64). Writes GPU command words. `[STANDARD]` for the Dynarmic part; the integration is `[HYPOTHESIS]` until the oracle test passes.
2. **GPU frontend** decodes command words into semantic operations tagged with the state IDs they read and write. `[DECISION]`
3. **ERG** places each state as `ACTIVE` or `CACHED` based on relevance. A cached state is reactivated only if its guard (version or dirty-range) still holds. `[DECISION-PENDING]` for the placement threshold and eviction policy.
4. **Hardware boundary specs** (HCT) mark an active state `QUARANTINED` when it violates a documented or measured boundary. `[DECISION]` for documented and measured specs; `[DECISION-PENDING]` for inferred specs.
5. **RSB** produces a candidate for a quarantined state. Execution continues on the quarantined contents, and the frame is tagged `SPECULATIVE`. The override is applied at a safe point. `[DECISION-PENDING]` for the safe-point definition and for frame presentation policy.
6. **Vulkan executor** consumes IR packets in order and submits them. It has no path to RSB. `[DECISION]`
7. **Reference path**: the same capture replayed without the tracker, ERG placement, RSB, or speculation. It is the only accuracy oracle. Accuracy means frame-hash equality with the reference. `[DECISION]`

## Hypotheses

- **H1** Reactivating a cached state with a passing guard yields the same frame hash as a fresh resolve.
- **H2** After an override, the frame hash converges to the reference within K frames. K is `[DECISION-PENDING]`.
- **H3** A documented boundary spec flags every state whose reference hash is wrong for that boundary.
- **H4** Presenting speculative frames costs less p99 frame time than holding them. Unmeasured.
- **H5** Guard checks are cheaper than re-resolving a state. Unmeasured.
- **H6** Re-recording invalidated command buffers is cheaper than a full frame re-record. Unmeasured.
- **H7** Relevance-driven cache placement does not change any frame hash. Tested by a differential comparison; see `docs/results/`.

Full statements and falsification tests are in RFC 0002.

## What is implemented

See [`docs/results/scaffold.md`](docs/results/scaffold.md) for the exact list. In short: capture format v1, reference replay, liveness pass (ADR 0003), RSB candidate resolver, a single-threaded and threaded frame loop, and comparator harnesses for specialized selection, placement policy, speculative override, and fault injection. Ampere, the Vulkan device path, and commercial-game loading are stubs that return `kNotImplemented`.

Liveness is the only pruning that is proven, and only for the cases in ADR 0003. Relevance is not yet decided: it may move a state to `CACHED`, but the claim that this never changes output is H7, not an invariant.

## Design decisions

Decisions are recorded in [`docs/decisions/`](docs/decisions/). Open decisions are listed in AGENTS.md §7 and RFC 0002.

## Scope

- **Platform:** desktop Vulkan only.
- **Inputs:** homebrew, original test ROMs, conformance suites, and user-owned captures. No tooling for dumping, decrypting, or extracting commercial game data or keys.
- **Targets:** Switch 1 (Maxwell) first. Switch 2 and Ampere are out of scope until the Semantic IR contract is frozen.
- **CPU profiles:** `switch1_a57` and `switch2_a78c` are hypothesis profiles that set an assumed guest feature set. They are not validated against hardware.

## Building

### Requirements

- CMake 3.25 or higher
- Clang 22.1.8 (tested on Linux x86_64; Clang 18 not tested), C++23
- Vulkan SDK
- GLFW 3

### Commands

```bash
git submodule update --init --recursive
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

Presets: `debug`, `release`, `relwithdebinfo`, `asan`, `ubsan`, `tsan`. TSan is not run for every target; see `docs/results/`.

Dynarmic is pinned as a submodule to the `Hikari-Emulator/dynarmic` fork at base commit `9d45823`, which originates from an unofficial mirror. See [`THIRD_PARTY.md`](THIRD_PARTY.md).

## Layout

See [`AGENTS.md`](AGENTS.md) §7.2.

## License

GPL-3.0. See [`LICENSE`](LICENSE).
