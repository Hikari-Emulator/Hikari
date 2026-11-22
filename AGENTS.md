# AGENTS.md — Hikari

This file defines the engineering contract for humans and automated agents working in this repository. It is normative. If code and this document disagree, fix one of them in the same change; do not leave the divergence.

Hikari is a research emulator. Its central claim (bounded speculative semantic reconstruction under hardware-derived constraints) is a **hypothesis**. Treat every claim about correctness or performance as unproven until a test in this repository demonstrates it.

---

## 0. Ground rules

- **Evidence over assertion.** Do not state that a component works, is faster, or is correct without a reproducible test, benchmark, or hash comparison that you ran and can point to.
- **Reference path is sacred.** The non-speculative reference path (capture → executor, no RSB, no ERG) must always build, run, and remain selectable at runtime. Never remove it, never route it through RSB/ERG/caches.
- **Scope.** Work only with homebrew, original test ROMs, conformance suites, and captures produced from hardware or software the contributor is entitled to use. Do not add tooling for dumping, decrypting, or extracting commercial game data or keys. Do not add code that circumvents platform protections.
- **Don't widen scope silently.** Switch 2 support, commercial game compatibility, and performance targets are out of scope for Phase 0–2 (see §9). Do not start them as side effects of other work.
- **Small, verifiable changes.** One logical change per commit. Each change states which phase and which invariant it affects.

---

## 1. Language, toolchain, and build

- **Language:** C++23.
- **Compilers:** Clang ≥ 18 (primary), GCC ≥ 13 (secondary, must build in CI).
- **Build:** CMake ≥ 3.25. Presets in `CMakePresets.json` are the only supported entry points: `debug`, `release`, `asan`, `ubsan`, `tsan`, `relwithdebinfo`.
- **Warnings:** `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wunused -Werror`. No `#pragma` suppression without a comment explaining why and a link to an issue.
- **Static analysis:** `clang-tidy` with the repo's `.clang-tidy` must pass with zero new findings. Do not add `NOLINT` without a reason on the same line.
- **Formatting:** `clang-format` with the repo's `.clang-format`. Unformatted code does not merge.
- **Sanitizers:** all test suites run under ASan+UBSan on every PR. TSan runs on every change that touches threading or queues.
- **Dependencies:** pinned by commit hash in `third_party/` or via CMake FetchContent with a hash check. No unpinned `main` branches. Each dependency has a line in `THIRD_PARTY.md` with license and purpose.

---

## 2. Coding style

### 2.1 General
- Prefer clarity over cleverness. A reviewer must be able to predict the behavior of a function from its signature and name.
- Functions should do one thing. If a function needs a comment to explain its sections, split it.
- No global mutable state. Pass context explicitly. Singletons are forbidden unless wrapped in an explicitly-initialized, explicitly-torn-down service with a documented lifetime.
- No `using namespace` in headers. Never in `.cpp` at file scope either unless inside a narrow function scope.
- Error handling: return `std::expected<T, Error>` (or the repo's `Result<T>` alias) for recoverable errors. Use `assert` / `HIKARI_ASSERT` for invariants. Use exceptions nowhere in the emulation core.
- Invariant violations in the core call `HIKARI_UNREACHABLE` / `HIKARI_CHECK`, which log state and abort in debug, and in release abort with a diagnostic capture dump.

### 2.2 Naming
- Types: `PascalCase`. Functions and variables: `snake_case`. Constants and `constexpr` values: `kPascalCase`. Macros: `HIKARI_SCREAMING_SNAKE`, and macros are discouraged (see §3.4).
- Members: trailing underscore (`cache_`). Struct data members: no underscore.
- Namespaces: `hikari::<module>`. No anonymous namespace in headers.
- Files: `snake_case.hpp` / `snake_case.cpp`. One primary type per header.

### 2.3 Headers
- Self-contained: every header compiles alone (enforced by a CI target).
- Include order: own header, C system, C++ standard, third-party, project. Separated by blank lines.
- Forward-declare where possible. Do not include `<vulkan/vulkan.h>` in headers that do not need it; wrap Vulkan behind `hikari::gfx` types.

### 2.4 Types and interfaces
- Use strong types for identifiers: `struct ResourceId { std::uint32_t index; std::uint32_t generation; };`, not raw `uint32_t`.
- Fixed-width integers for anything that crosses a serialization or ABI boundary. `std::size_t` only for container sizes.
- `enum class` always. Exhaustive `switch` with no `default:` on project enums, so the compiler flags missing cases.
- `[[nodiscard]]` on all functions returning results, handles, or validation outcomes.
- `noexcept` on destructors, move operations, and every function in the hot path that cannot throw by construction.
- Prefer `std::span` and `std::string_view` for non-owning parameters.

### 2.5 Const and mutability
- `const` by default for locals and parameters where the value is not modified.
- `constexpr` for every compile-time constant, including tables.

---

## 3. Metaprogramming policy

Metaprogramming is a tool for **removing runtime dispatch and encoding invariants at compile time**, not for showing technique. Each use must justify itself by one of: eliminating a hot-path branch, making an invariant unrepresentable, or generating verified data from a single source.

### 3.1 Allowed
- **Policy-based templates for GPU frontends.** `template <class Gen> class GpuFrontend` where `Gen` satisfies a `GpuGeneration` concept (see §3.2). Maxwell and Ampere are instantiated, not virtually dispatched, in the hot path. [DECISION-PENDING]
- **`constexpr` tables** for the Hardware Capability Table when the source data is static. Table validation (ranges, monotonicity, no duplicate keys) runs at compile time via `static_assert`. [DECISION-PENDING]
- **Concepts** to constrain every template parameter. Unconstrained template parameters are rejected in review. [DECISION-PENDING]
- **Code generation from data.** The HCT source of truth is `data/hct/*.toml`. A generator (`tools/hctgen`) produces `hct_generated.hpp` with a checked-in output and a CI check that the generated file matches the source. Generated files are never hand-edited. [DECISION-PENDING]
- **`if constexpr`** for generation-specific branches inside a single template, instead of duplicating code. [STANDARD]
- **Type-level state encoding** where it removes runtime checks, e.g., a `Recording` vs `Recorded` capture type so that replay cannot be called on an open capture. [DECISION-PENDING]
- **Allowed with ADR, not yet implemented:** [DECISION-PENDING]
  - `constexpr` transition table, validated by `static_assert` that every `(state, event)` pair is either listed or explicitly marked illegal. Note: this checks the table's completeness as a documentation/table check only, not the runtime behavior or design validity. [DECISION-PENDING]
  - Concepts that constrain which state kinds may be admitted into the ERG. [DECISION-PENDING]
  - Typestate: [HYPOTHESIS] (`tests/unit/lifecycle_typestate_test`, metric: count of compile-time vs runtime-checked transition sites) that typestate is usable for transitions. State held in containers and transitioned by events loses static guarantees at the container boundary; the compile-time guarantee ends where a state is erased into a homogeneous container (`std::vector<SemanticState>` or pool slot) and runtime event dispatch takes over.
- **Not allowed without RFC:** expression templates, CRTP hierarchies deeper than one level, reflection-based serialization, template recursion deeper than needed by the concept. [DECISION-PENDING]

### 3.2 The frontend contract (concept)
```cpp
template <class G>
concept GpuGeneration = requires {
    typename G::CommandWord;
    typename G::CaptureEntry;
    { G::kName } -> std::convertible_to<std::string_view>;
    { G::kHctTable } -> std::convertible_to<const HctTable&>;
    { G::decode(std::declval<const typename G::CommandWord*>(),
                std::declval<CaptureSink&>()) } -> std::same_as<DecodeResult>;
};
```
Every generation must satisfy this concept. Adding a field to the contract requires an RFC in `docs/rfcs/` and a version bump of the IR (§6.4).

### 3.3 Forbidden
- Template recursion deeper than necessary; prefer `std::index_sequence` or fold expressions.
- SFINAE where a concept suffices.
- Expression templates or CRTP hierarchies that a reviewer cannot read in one pass.
- Runtime reflection or string-keyed registries in the core.
- `dynamic_cast` and `typeid` in the core.
- Compile-time code generation that depends on wall-clock time, host paths, or unordered iteration.
- Macros for anything other than include guards, feature gating, and the assert/log wrappers.

### 3.4 Macros
Allowed: include guards (`#pragma once` is acceptable), `HIKARI_ASSERT`, `HIKARI_LOG`, platform detection. Anything else must be an `inline constexpr` function or a template.

---

## 4. Memory management

### 4.1 Principles
1. **No heap allocation on the hot path.** The per-frame path (decode → RSB → ERG → IR emit → executor submit) allocates from pre-sized arenas or pools. Any allocation in that path is a bug unless marked `// HOT-ALLOC-OK: <reason>` and listed in a tracked exception file.
2. **Ownership is explicit and single.** Every resource has one owner. Non-owning access uses `std::span`, `std::string_view`, or a handle (ID + generation), never a raw pointer that outlives its owner.
3. **Handles over pointers** for anything that lives in the IR, the ERG, or the capture. Pointers invalidate on arena reset; handles are checked.
4. **Determinism of allocation does not matter for correctness, but determinism of layout does** for replay hashing. Hashes are computed over semantic content, never over raw memory.

### 4.2 Allocators
- **Frame arena:** bump allocator, reset at frame end. Used for IR nodes, ERG nodes, speculative candidates. Size fixed at startup per profile; overflow is a fatal diagnostic, not a silent grow.
- **Capture arena:** append-only, lives for one capture session. Serialized to disk in capture format (§6.1).
- **Pools:** fixed-size object pools for pipelines, descriptor sets, and command buffers. Free-list with generation counters for handle validation.
- **Persistent cache:** memory-mapped, read-only after load. Versioned with a header and checksum.
- All allocators implement `std::pmr::memory_resource` or a project-local equivalent, so containers can be parameterized. Containers on the hot path use `std::pmr::vector` backed by a frame arena, not the default heap.

### 4.3 Layout
- Hot data is structure-of-arrays where access patterns are columnar (ERG node scoring, guard checks). Array-of-structures is acceptable for cold or per-object data.
- Cache-line alignment (`alignas(64)`) for any structure touched by more than one thread.
- Avoid false sharing: per-thread counters are padded to cache-line size.
- Check `sizeof` and `alignof` for IR and capture entries with `static_assert`. Layout changes are visible in review.

### 4.4 Ownership of GPU and Vulkan resources
- Vulkan handles are wrapped in move-only RAII types (`gfx::Buffer`, `gfx::Image`, `gfx::Pipeline`, …). No raw `VkX` handles escape `gfx/` except in the Vulkan backend source files.
- Destruction order follows a defined deletion queue per frame-in-flight. Resources are released only after the GPU has finished using them (fence-tracked), never on the CPU's schedule.
- Every resource has a debug name. Leak detection runs at shutdown in debug builds and fails tests.

### 4.5 Guest memory
- Guest address space is a reserved virtual range. Accesses go through a translation layer. Where the platform permits, use hardware-assisted fast paths (fastmem); the slow path is always available and used in tests.
- Guest-to-host translation returns `Result`, not a raw pointer.
- Guest memory writes that affect GPU-visible resources are tracked (dirty ranges), and the tracking is part of the context model (§7.5).

---

## 5. Concurrency

### 5.1 Threads
- **CPU thread:** guest CPU (Dynarmic). Produces GPU command words into a queue.
- **GPU frontend thread:** consumes command words, emits capture entries and semantic IR.
- **Executor thread:** owns the Vulkan device context. Only this thread calls Vulkan.
- **Validator thread (optional):** consumes reference-path outputs and compares hashes.
- **Services thread(s):** HLE services, isolated from GPU state.

### 5.2 Rules
- Each thread owns its mutable state. Cross-thread communication uses SPSC or MPSC queues with explicit capacity. No shared mutable state without a documented synchronization primitive.
- Memory ordering is explicit (`std::memory_order_*`). `memory_order_seq_cst` is the default only where justified in a comment.
- No blocking calls (locks, waits) in the GPU frontend hot path. Backpressure is handled by bounded queues and explicit stall counters that appear in metrics.
- Lock hierarchy, when locks are unavoidable, is documented in `docs/concurrency.md`. Violations are caught by TSan in CI.

---

## 6. Data formats and determinism

### 6.1 Capture format (v1)
- Binary, little-endian, with a header containing: magic, format version, generation ID, frame count, a SHA-256 of the body.
- Entries are fixed-size records with a type tag, followed by variable-length payloads with explicit lengths.
- A capture is a **pure function of its input**. No timestamps, no host paths, no pointers, no iteration over unordered containers.
- Captures from homebrew or original test ROMs are committed under `tests/captures/` only if licensed for redistribution. Otherwise they are generated locally by a documented script.

### 6.2 Determinism requirements
- Same capture + same configuration → identical output hashes across three consecutive runs. Enforced by Phase 0 test.
- Hash inputs are semantic content (ordered), not memory bytes.
- RSB guesses are deterministic given (capture, HCT version, ERG state, seed). The seed is recorded in the run manifest.
- Hash function: XXH3-128 for frame hashes, SHA-256 for capture integrity.

### 6.3 Hashing and comparison
- A frame hash is computed over the rendered render target(s) per region (tile grid defined in config), plus the IR hash for the frame.
- Mismatches report: frame index, region, expected and actual hash, the IR node set active in that region, and the guard status of speculative nodes.

### 6.4 Semantic IR versioning
- IR has a version number `MAJOR.MINOR`. Breaking changes bump MAJOR and require an RFC.
- Every IR node carries: `id`, `version`, `kind`, dependency list, and (if speculative) `guard` and `spec_id`.
- Speculative nodes carry status: `pending`, `verified`, `invalid`. Transitions are logged.
- The executor rejects IR with an unknown MAJOR version. It does not attempt to interpret it.

---

## 7. Architecture

### 7.1 Runtime model

1. **Guest CPU execution (Owner: CPU thread).** Guest CPU (Dynarmic) executes guest code. Input: guest memory image. Output: GPU command words written to the command queue. [STANDARD]
2. **Command decode (Owner: GPU frontend thread).** GPU frontend decodes command words into semantic operations, and emits them into the current frame's operation list. Input: GPU command words from the command queue. Output: operations tagged with the state IDs they read and write. [DECISION-PENDING]
3. **Relevance tracking (Owner: GPU frontend thread, or dedicated tracker thread).** State tracker updates each state's relevance inputs after every frame (see §7.4). Input: frame operation list and state access counts. Output: updated per-state relevance inputs. [DECISION-PENDING]:
   - *Option A (tracker on GPU frontend thread):* zero cross-thread synchronization cost between decode and tracking, but increases frontend thread per-frame latency. [DECISION-PENDING]
   - *Option B (tracker on its own thread):* requires an SPSC/MPSC event queue or double-buffered relevance snapshot per frame and explicit memory ordering between the tracker thread and ERG placement on the frontend thread. [DECISION-PENDING]
4. **ERG placement (Owner: GPU frontend thread).** ERG decides placement: `ACTIVE` or `CACHED`. Input: per-state relevance inputs and placement threshold. Output: lifecycle state placement (`ACTIVE` or `CACHED`). Placement is a function of relevance inputs and the placement threshold. [DECISION-PENDING]: threshold function form and window length `N` (see §7.4).
5. **Cached state reactivation (Owner: GPU frontend thread).** On a request for a cached state, ERG runs the guard check (§7.5) and either reactivates the state or marks it `STALE`. Input: `CACHED` state and its attached `Guard`. Output: `ACTIVE` state (on guard pass) or `STALE` state (on guard failure). [DECISION-PENDING]
6. **Hardware boundary check (Owner: GPU frontend thread).** ERG checks each `ACTIVE` state against the hardware boundary specs (§7.6). Input: `ACTIVE` state and generation boundary specs. Output: `ACTIVE` state (if all predicates hold) or transition to `QUARANTINED` plus a resolution request submitted to RSB (on violation). [DECISION-PENDING]
7. **RSB candidate generation (Owner: GPU frontend thread / RSB worker; see §5 conflict note in §7.12).** RSB produces a candidate for a quarantined state. Input: quarantined state, boundary violation ID, and context. Output: speculative `Candidate` with `spec_id`. Execution continues with the quarantined state's current contents, and the frame carries a speculation marker. [DECISION-PENDING]
8. **Override scheduling and application (Owner: GPU frontend thread).** When RSB returns, the override is recorded (§7.7) and applied at a synchronization boundary. Input: RSB `Candidate` and override record. Output: state transitioned from `QUARANTINED` to `OVERRIDDEN`, then to `ACTIVE` when applied at the synchronization boundary. [DECISION-PENDING]: definition of the synchronization boundary (end of current render pass vs next frame boundary; see §7.7).
9. **IR emission (Owner: GPU frontend thread).** Frame is emitted as IR to the executor queue, tagged `FINAL` or `SPECULATIVE` (§7.8, §7.9). Input: frame operation list and active/speculative state set. Output: immutable IR packet queued for the executor thread. [DECISION-PENDING]
10. **Vulkan execution (Owner: Executor thread).** Vulkan executor consumes IR packets in order, records command buffers, and submits. Input: ordered IR packets from the executor queue. Output: submitted Vulkan command buffers and frame output. It makes no semantic decisions. [DECISION-PENDING]

### 7.2 Module layout and data model

```text
src/
  core/        assert, log, result, arena, pool, handles, hashing
  capture/     capture format v1 writer, reader, synthetic fixtures (added in Phase 0)
  loader/      image parsing, memory layout
  cpu/         Dynarmic integration, interpreter fallback
  services/    HLE services (minimal, test-driven)
  gpu/
    frontend/  template <Gen> frontend, capture writer, state tracker
    gen/       maxwell/, ampere/ (decode tables, command words)
    hct/       generated HCT / hardware boundary specs, generator glue
  rsb/         bounded hypothesis generation, candidate resolution
  erg/         placement (ACTIVE/CACHED), guard checks, boundary checks, lifecycle state machine
  ir/          semantic IR definitions, packet serialization, versioning
  exec/        executor interface, Vulkan backend (gfx/)
  validate/    reference path, hashing, comparison, metrics
  checkpoint/  snapshot, override logging, and rollback
tools/
  hctgen/      HCT generator
  capdump/     capture inspection and fixture output
tests/
  unit/ integration/ golden/ fault/ bench/
data/
  hct/         TOML source of truth
docs/
  rfcs/ concurrency.md decisions/ (ADRs) results/
```

#### Data model structures

- **`SemanticState`:** [DECISION-PENDING]
  - `id`: `StateId` (`index: uint32`, `generation: uint32`). Invariant: `generation` increments on slot reuse; `index` addresses the state pool. [STANDARD]
  - `kind`: enum `{Surface, Buffer, Texture, ShaderBinding, RasterState, BlendState, DepthState, Other}`. Invariant: exhaustive enum with no unspecified numeric values. [DECISION-PENDING]
  - `format`: format identifier from the capture's format table. Invariant: must map to a format entry in the capture or `kUnknown` prior to initialization. [DECISION-PENDING]
  - `dimensions`: per-kind extent (`width`, `height`, `depth` or byte length for `Buffer`; zeroed for non-dimensional state kinds). [DECISION-PENDING]
  - `producer_op`: `uint32` index of the operation that last wrote this state. Invariant: monotonically increases on each write within a capture stream. [DECISION-PENDING]
  - `content_hash`: `XXH3-128` computed over semantic fields only, not guest memory bytes. [DECISION-PENDING]
  - `lifecycle`: `LifecycleState` enum (`{ACTIVE, CACHED, STALE, QUARANTINED, OVERRIDDEN}`, §7.4). [DECISION-PENDING]
  - `lifecycle_entered_frame`: `FrameIndex` (`uint64`) of the last lifecycle transition. [DECISION-PENDING]
  - `quarantine_reason`: `BoundaryId` or `none`. Invariant: non-empty if and only if `lifecycle == QUARANTINED` or transitioning through override from quarantine. [DECISION-PENDING]
  - `guard`: `Guard` variant (§7.5). Invariant: populated whenever `lifecycle == CACHED` or `speculation_id != none`. [DECISION-PENDING]
  - `speculation_id`: `SpecId` (`uint64`) or `none`. Invariant: non-empty while a state is under active speculation prior to override application. [DECISION-PENDING]

- **`Guard` (variant, three kinds only):** [DECISION-PENDING]
  - `VersionGuard {resource_id, expected_version}`: valid while the resource's current version in the context model equals `expected_version`. [DECISION-PENDING]
  - `DirtyRangeGuard {guest_range_start, guest_range_end, epoch}`: valid while no guest memory write has marked `[guest_range_start, guest_range_end)` dirty after `epoch`, with `guest_range_start <= guest_range_end`. [DECISION-PENDING]
  - `SyncGuard {barrier_op_index}`: valid only if no barrier between producer and consumer was removed or reordered relative to `barrier_op_index`. [DECISION-PENDING]
  - Any guard kind not listed above requires an RFC in `docs/rfcs/`. [DECISION-PENDING]

### 7.3 Invariants

1. Capture is a deterministic function of its input. [DECISION-PENDING]
2. Hardware boundary specs with `confidence = documented` or `measured` flag only states that cannot exist on the target hardware. [DECISION-PENDING]
3. Note on former Invariant 3 (not an invariant): relevance may move a state to `CACHED`, [DECISION-PENDING]; the claim that this never changes frame output is [HYPOTHESIS] H7 (`tests/golden/h7_relevance_placement_diff_test`, metric: per-frame and per-region hash mismatch count vs reference path).
4. Every speculative state and override is traceable to its `spec_id`, `guard`, `quarantine_reason`, and candidate set. [DECISION-PENDING]
5. The executor makes no semantic decisions. [DECISION-PENDING]
6. The reference path is independent of the state tracker, ERG placement, RSB, speculative caches, and override state. [DECISION-PENDING]
7. Side-effecting operations are never removed. [DECISION-PENDING]
8. IR packets are immutable after emission. [DECISION-PENDING]
9. Frame output is a function of `(capture, configuration, seed)`. Non-determinism is a test failure. [DECISION-PENDING]

### 7.4 Lifecycle state machine

States: `ACTIVE`, `CACHED`, `STALE`, `QUARANTINED`, `OVERRIDDEN`. [DECISION-PENDING]

Trigger events (8 total): `PLACEMENT_EVICT`, `REQUEST_HIT`, `CACHE_CAPACITY`, `BOUNDARY_VIOLATION`, `BOUNDARY_VIOLATION_INFERRED`, `RSB_RESOLVED`, `RSB_FAILED`, `OVERRIDE_APPLIED`. [DECISION-PENDING]

| From | To | Trigger | Condition | Side effects | Tag |
|---|---|---|---|---|---|
| `ACTIVE` | `CACHED` | `PLACEMENT_EVICT` | relevance score below threshold for `N` consecutive frames; no pending speculation on the state | move to cache index; keep guard | [DECISION-PENDING] for `N` and threshold function |
| `CACHED` | `ACTIVE` | `REQUEST_HIT` | guard check passes | reuse cached content; increment use counter | [DECISION-PENDING] |
| `CACHED` | `STALE` | `REQUEST_HIT` | guard check fails | discard cached content; log reason | [DECISION-PENDING] |
| `CACHED` | `STALE` | `CACHE_CAPACITY` | cache over budget; selected by eviction policy | discard; log | [DECISION-PENDING] for eviction policy (LRU vs hotness-weighted LRU) |
| `ACTIVE` | `QUARANTINED` | `BOUNDARY_VIOLATION` | predicate of a documented or measured boundary is false | record boundary ID; submit RSB request | [DECISION-PENDING] |
| `ACTIVE` | `QUARANTINED` | `BOUNDARY_VIOLATION_INFERRED` | boundary `confidence = inferred` | record boundary ID or log only | [DECISION-PENDING]: inferred boundaries quarantine, or only log? |
| `QUARANTINED` | `OVERRIDDEN` | `RSB_RESOLVED` | candidate passes bound check | record override; schedule at synchronization boundary | [DECISION-PENDING] |
| `QUARANTINED` | `QUARANTINED` | `RSB_FAILED` | RSB returns no valid candidate | keep current contents; apply retry policy | [DECISION-PENDING]: retry count, and what happens after the final failure |
| `OVERRIDDEN` | `ACTIVE` | `OVERRIDE_APPLIED` | at synchronization boundary | replace contents; clear `speculation_id` | [DECISION-PENDING] |

Any `(from, event)` pair not in this table is an illegal transition. Debug builds assert (`HIKARI_ASSERT`). Release builds log and refuse the transition. [DECISION-PENDING]

**Missing pairs (documentation completeness listing of the 32 remaining `(state, event)` pairs out of `5 states × 8 events = 40` total combinations; note `CACHED + REQUEST_HIT` has 2 conditional rows in the table above, covering 8 distinct `(state, event)` pairs; this listing is a documentation check only and does not verify the design):** [DECISION-PENDING]
- **From `ACTIVE` (5 unlisted events):** [DECISION-PENDING]
  - `(ACTIVE, REQUEST_HIT)`: state is already `ACTIVE`; whether a read hit on an `ACTIVE` state is a no-op that increments hotness or an illegal transition event is [DECISION-PENDING] (Option 1: no-op + hotness update; Option 2: `REQUEST_HIT` is emitted only for `CACHED` states and asserts on `ACTIVE`).
  - `(ACTIVE, CACHE_CAPACITY)`: illegal transition (debug assert, release log and refuse); capacity eviction applies only to `CACHED` states. [DECISION-PENDING]
  - `(ACTIVE, RSB_RESOLVED)`: illegal transition (debug assert, release log and refuse); `ACTIVE` state has no open quarantine request. [DECISION-PENDING]
  - `(ACTIVE, RSB_FAILED)`: illegal transition (debug assert, release log and refuse). [DECISION-PENDING]
  - `(ACTIVE, OVERRIDE_APPLIED)`: illegal transition (debug assert, release log and refuse). [DECISION-PENDING]
- **From `CACHED` (6 unlisted events):** [DECISION-PENDING]
  - `(CACHED, PLACEMENT_EVICT)`: illegal transition (debug assert, release log and refuse); state is already `CACHED`. [DECISION-PENDING]
  - `(CACHED, BOUNDARY_VIOLATION)`: illegal transition (debug assert, release log and refuse); boundary checks run only on `ACTIVE` states (Step 6). [DECISION-PENDING]
  - `(CACHED, BOUNDARY_VIOLATION_INFERRED)`: illegal transition (debug assert, release log and refuse). [DECISION-PENDING]
  - `(CACHED, RSB_RESOLVED)`: illegal transition (debug assert, release log and refuse). [DECISION-PENDING]
  - `(CACHED, RSB_FAILED)`: illegal transition (debug assert, release log and refuse). [DECISION-PENDING]
  - `(CACHED, OVERRIDE_APPLIED)`: illegal transition (debug assert, release log and refuse). [DECISION-PENDING]
- **From `STALE` (all 8 events: `PLACEMENT_EVICT`, `REQUEST_HIT`, `CACHE_CAPACITY`, `BOUNDARY_VIOLATION`, `BOUNDARY_VIOLATION_INFERRED`, `RSB_RESOLVED`, `RSB_FAILED`, `OVERRIDE_APPLIED`):** [DECISION-PENDING]
  - Whether `STALE` is a terminal tombstoned state (all 8 events illegal, slot reclaimed by pool generation bump) or allows re-initialization on a new producer write is [DECISION-PENDING] (Option 1: terminal state, all 8 events assert in debug and refuse in release, new writes allocate a new `StateId` generation; Option 2: add a `REINITIALIZE` transition from `STALE` to `ACTIVE`).
- **From `QUARANTINED` (6 unlisted events):** [DECISION-PENDING]
  - `(QUARANTINED, PLACEMENT_EVICT)`: illegal transition (debug assert, release log and refuse); states with pending speculation/quarantine cannot be evicted to `CACHED`. [DECISION-PENDING]
  - `(QUARANTINED, REQUEST_HIT)`: how reads of a `QUARANTINED` state while RSB is still running are modeled is [DECISION-PENDING] (Option 1: no lifecycle event is fired and execution reads current contents per Step 7; Option 2: self-transition `(QUARANTINED, REQUEST_HIT) -> QUARANTINED` that increments hotness and attaches the frame's speculation marker).
  - `(QUARANTINED, CACHE_CAPACITY)`: illegal transition (debug assert, release log and refuse). [DECISION-PENDING]
  - `(QUARANTINED, BOUNDARY_VIOLATION)`: whether a second boundary violation on an already-`QUARANTINED` state is coalesced or queued is [DECISION-PENDING] (Option 1: coalesce/ignore while `QUARANTINED`; Option 2: append `BoundaryId` to a per-state violation list).
  - `(QUARANTINED, BOUNDARY_VIOLATION_INFERRED)`: [DECISION-PENDING] (same options as `(QUARANTINED, BOUNDARY_VIOLATION)`).
  - `(QUARANTINED, OVERRIDE_APPLIED)`: illegal transition (debug assert, release log and refuse); state must transition to `OVERRIDDEN` via `RSB_RESOLVED` first. [DECISION-PENDING]
- **From `OVERRIDDEN` (7 unlisted events):** [DECISION-PENDING]
  - `(OVERRIDDEN, PLACEMENT_EVICT)`: illegal transition (debug assert, release log and refuse); pending override must be applied before eviction. [DECISION-PENDING]
  - `(OVERRIDDEN, REQUEST_HIT)`: behavior when an operation reads an `OVERRIDDEN` state before the synchronization boundary is reached is [DECISION-PENDING] (Option 1: read pre-override contents and mark frame `SPECULATIVE` until `OVERRIDE_APPLIED`; Option 2: force immediate synchronization boundary if within the same command list).
  - `(OVERRIDDEN, CACHE_CAPACITY)`: illegal transition (debug assert, release log and refuse). [DECISION-PENDING]
  - `(OVERRIDDEN, BOUNDARY_VIOLATION)`: [DECISION-PENDING] (Option 1: illegal because boundary checks only run after `OVERRIDE_APPLIED` returns the state to `ACTIVE`; Option 2: transition back to `QUARANTINED` if pre-override contents trigger another boundary check).
  - `(OVERRIDDEN, BOUNDARY_VIOLATION_INFERRED)`: [DECISION-PENDING] (same options as `(OVERRIDDEN, BOUNDARY_VIOLATION)`).
  - `(OVERRIDDEN, RSB_RESOLVED)`: illegal transition (debug assert, release log and refuse); override is already resolved. [DECISION-PENDING]
  - `(OVERRIDDEN, RSB_FAILED)`: illegal transition (debug assert, release log and refuse). [DECISION-PENDING]

### 7.5 Guard checks and reactivation

- A cached state carries its guard from the moment it is evicted (`ACTIVE -> CACHED`). [DECISION-PENDING]
- `DirtyRangeGuard` is checked against the memory layer's dirty-range log since `epoch`. The dirty-range log format is [DECISION-PENDING]:
  - *Option A (per-page bitmap with epoch counter per page):* $O(1)$ bit test per 4 KiB page in range and fixed memory overhead proportional to guest address space size, but coarse 4 KiB false-positive granularity for small buffers. [DECISION-PENDING]
  - *Option B (sorted interval tree / append-only interval list per epoch):* exact byte-range precision and memory proportional to write count since `epoch`, but $O(\log M + K)$ lookup cost per guard check and requires arena compaction on epoch rollover. [DECISION-PENDING]
- `VersionGuard` is checked against the resource's current monotonic version in the context model (`current_version == expected_version`). [DECISION-PENDING]
- `SyncGuard` is checked against the frame's barrier ordering log (`barrier_op_index` preserved between producer and consumer). [DECISION-PENDING]
- A reactivation (`CACHED -> ACTIVE` on `REQUEST_HIT`) that passes every guard must still re-verify the `content_hash` of the restored state against the recorded `content_hash`. A mismatch is treated as a guard failure (`CACHED -> STALE`). [DECISION-PENDING]
- Guard check cost is unmeasured. [HYPOTHESIS] H5 (`tests/bench/h5_guard_check_bench`, metric: guard check time per request, p99, on the test corpus vs fresh state resolution time).

### 7.6 Hardware boundary specs

Each hardware boundary spec record contains: [DECISION-PENDING]
- `boundary_id`: strong identifier (`BoundaryId`). [STANDARD]
- `generation`: target GPU generation (`Maxwell`, `Ampere`, or other). [DECISION-PENDING]
- `predicate`: a machine-checkable boolean expression over `SemanticState` fields (`kind`, `format`, `dimensions`, `binding_flags`), never free text. [DECISION-PENDING]
- `source`: hardware documentation reference, measurement test ID, or `"none"`. [DECISION-PENDING]
- `confidence`: enum `{documented, measured, inferred}`. [DECISION-PENDING]
- `verified_by`: repository test ID or `none`. [DECISION-PENDING]

Rules: [DECISION-PENDING]
- A boundary spec with `confidence = documented` or `confidence = measured` may quarantine an `ACTIVE` state (`ACTIVE -> QUARANTINED`). [DECISION-PENDING]
- Whether a boundary spec with `confidence = inferred` may not quarantine a state until promoted to `measured` or `documented` is [DECISION-PENDING] (Option 1: `inferred` specs never quarantine and only emit diagnostic counters; Option 2: `inferred` specs quarantine in experimental mode only).
- A boundary spec only answers "can this state exist on this hardware." It never answers "does this emulation match the hardware." That question is answered only by the reference path (§7.10). [DECISION-PENDING]

### 7.7 Override semantics

Each override record contains: [DECISION-PENDING]
- `spec_id`: `SpecId` of the RSB resolution. [DECISION-PENDING]
- `state_id`: `StateId` of the quarantined state being overridden. [DECISION-PENDING]
- `frame_index_at_quarantine`: `FrameIndex` when `ACTIVE -> QUARANTINED` occurred. [DECISION-PENDING]
- `frame_index_at_override`: `FrameIndex` when `OVERRIDDEN -> ACTIVE` (`OVERRIDE_APPLIED`) occurred. [DECISION-PENDING]
- `hash_before`: `XXH3-128` `content_hash` of the quarantined state prior to override. [DECISION-PENDING]
- `hash_after`: `XXH3-128` `content_hash` of the state after applying the RSB candidate. [DECISION-PENDING]
- `affected_regions`: list of render target tile-grid regions (§6.3) touched by operations that read `state_id` while speculative. [DECISION-PENDING]
- `speculative_frames`: list of `(FrameIndex, FrameStatus)` markers for every frame that used `state_id` between `frame_index_at_quarantine` and `frame_index_at_override`. [DECISION-PENDING]

Override timing: an override is applied only at a synchronization boundary. The choice of synchronization boundary is [DECISION-PENDING]:
- *Option 1 (end of the current render pass that reads the state):* lower latency (override takes effect within the same frame for subsequent render passes; 0 additional frames carry a stale speculation marker if RSB resolves before render-pass end), but requires splitting IR packets at render-pass boundaries and invalidating mid-frame pipeline bindings. [DECISION-PENDING]
- *Option 2 (next frame boundary):* simpler frame-atomic state replacement and IR emission, at the cost of at least 1 full frame of latency (minimum 1 to $M$ frames carrying a stale speculation marker while RSB runs across frame boundaries). [DECISION-PENDING]

Frames already presented while speculative are not retroactively changed; they are recorded as `presented-speculative` in the override log. [DECISION-PENDING]

### 7.8 Frame status and presentation

Each frame carries a status enum: [DECISION-PENDING]
- `CLEAN`: no speculative or quarantined states were read during the frame. [DECISION-PENDING]
- `SPECULATIVE`: at least one speculative/quarantined state was read during the frame and has not yet been overridden. [DECISION-PENDING]
- `RESOLVED_PENDING_RECORD`: all speculative states used by the frame have been overridden after use, and the override log entry is pending commit. [DECISION-PENDING]

Presentation policy for `SPECULATIVE` frames is [DECISION-PENDING]:
- *Option 1 (present `SPECULATIVE` frames with a visual/diagnostic region marker):* zero presentation stall latency, at the cost of visible regional artifacts on screen; measurable by per-region hash diff count against the reference path. [DECISION-PENDING]
- *Option 2 (hold `SPECULATIVE` frames until RSB resolution and override application):* prevents presenting unverified speculative regions, at the cost of added presentation latency (unbounded if RSB resolution stalls or fails); measurable by p99 frame time and stall duration. [DECISION-PENDING]
- *Option 3 (present `SPECULATIVE` frames without marking):* listed for completeness only; rejected for research output because it hides speculative regions and prevents attribution of mismatches (`docs/decisions/0001-hypothesis-framing.md`). [DECISION] (`docs/decisions/0001-hypothesis-framing.md`)

### 7.9 Vulkan executor contract

Each IR packet sent from the GPU frontend thread to the executor thread contains: [DECISION-PENDING]
- `packet_id`: monotonically increasing `uint64` packet sequence number. [DECISION-PENDING]
- `frame_index`: `FrameIndex` (`uint64`) to which the packet belongs. [DECISION-PENDING]
- `status`: packet tag `{FINAL, SPECULATIVE}`. [DECISION-PENDING]
- `referenced_spec_ids`: span of `SpecId` values read by operations in this packet. [DECISION-PENDING]
- `operations`: ordered span of versioned Semantic IR operations (§6.4). [DECISION-PENDING]

Executor rules: [DECISION-PENDING]
- Consumes packets in strict `packet_id` order; out-of-order arrival is a fatal invariant error. [DECISION-PENDING]
- Never mutates an IR packet after receipt. [DECISION-PENDING]
- Pipeline cache key is the semantic hash (`XXH3-128`) of the pipeline-relevant state. [DECISION-PENDING]
- On override (`OVERRIDE_APPLIED`), every recorded command buffer that references the overridden `spec_id` is marked invalid and re-recorded. Re-record cost is unmeasured: [HYPOTHESIS] H6 (`tests/bench/h6_command_buffer_rerecord_bench`, metric: re-record time per override, p99, on the test corpus).
- The executor has no path to RSB and never invokes RSB. [DECISION-PENDING]

### 7.10 Reference path as the only accuracy oracle

- The reference path runs the same capture with no state tracker, no ERG `CACHED` placement, no RSB, and no speculation. [DECISION-PENDING]
- Accuracy of any run is defined strictly as frame-hash equality with the reference path, per frame and per region (§6.3). [DECISION] (`docs/decisions/0001-hypothesis-framing.md`)
- An unresolved read in the capture (a state read before any write, with no capture entry that defines it) is a reported error (`ErrorCode::kInvalidArgument`) in the reference path, never a guess. [DECISION-PENDING]

### 7.11 Hypotheses with falsification tests

| ID | Statement | Falsifying Test ID | Metric | Pass Criterion | Tag |
|---|---|---|---|---|---|
| **H1** | "H1: reactivation of a `CACHED` state whose guard passes yields the same frame hash as a fresh resolve from the capture." | `tests/golden/h1_cached_reactivation_diff_test` | Per-frame and per-region `XXH3-128` hash mismatch count between cached-reactivated run and fresh-resolve reference run | `0` hash mismatches across all frames and regions in the test corpus | [HYPOTHESIS] |
| **H2** | "H2: after an override is applied at the synchronization boundary, the frame hash converges to the reference hash within `K` frames." (`K` is [DECISION-PENDING]) | `tests/fault/h2_override_convergence_test` | Number of frames after `frame_index_at_override` until per-frame hash equals reference path hash | Convergence frame delta $\le K$ for 100% of injected/quarantined overrides | [HYPOTHESIS] |
| **H3** | "H3: a documented boundary spec flags every state whose reference-path hash is wrong for that boundary." | `tests/fault/h3_boundary_coverage_test` | Count of states with a reference-path mismatch on a documented boundary where the boundary predicate evaluated to true (unflagged) | `0` unflagged states among states causing a boundary mismatch | [HYPOTHESIS] |
| **H4** | "H4: presenting `SPECULATIVE` frames costs less p99 frame time than holding them." (Unmeasured) | `tests/bench/h4_speculative_presentation_bench` | p99 frame time (ms) under present-marked policy vs hold-until-resolved policy on identical capture + seed | `p99(present_marked) < p99(hold_until_resolved)` on pinned benchmark profile | [HYPOTHESIS] |
| **H5** | "H5: guard checks are cheaper than re-resolving the state." (Unmeasured) | `tests/bench/h5_guard_check_bench` | p99 guard check time per request (ns) vs p99 fresh state decode/resolve time (ns) on the test corpus | `p99(guard_check) < p99(fresh_resolve)` on pinned benchmark profile | [HYPOTHESIS] |
| **H6** | "H6: re-recording invalidated command buffers costs less than a full frame re-record." (Unmeasured) | `tests/bench/h6_command_buffer_rerecord_bench` | p99 command-buffer re-record time per override (us) vs p99 full-frame command-buffer record time (us) | `p99(override_rerecord) < p99(full_frame_record)` on pinned benchmark profile | [HYPOTHESIS] |
| **H7** | "H7: relevance-driven `CACHED` placement does not change the frame hash of any frame." | `tests/golden/h7_relevance_placement_diff_test` | Differential frame-hash mismatch count between relevance-caching enabled and reference path | `0` frame-hash mismatches across the entire test corpus | [HYPOTHESIS] |

### 7.12 Conflicts with §8.2 phase gates and §5 concurrency

Do not edit §8.2 or §5 silently; the table below records every conflict between the runtime model in §7.1–§7.11 and the rest of this document: [DECISION-PENDING]

| Location | Existing Text | Conflict with §7.1–§7.11 Runtime Model | Status |
|---|---|---|---|
| **§8.2 Phase 0** (line 337) & `docs/results/phase0.md` | "Phase 0: replay of one deterministic capture produces identical hashes across 3 runs." | Phase 0 and `docs/results/phase0.md` test only capture format v1 and the stateless operation-stream reference replay; they do not cover the state tracker, `ACTIVE`/`CACHED` lifecycle placement, boundary quarantine, overrides, or guards (`VersionGuard`, `DirtyRangeGuard`, `SyncGuard`). | [DECISION-PENDING] |
| **§8.2 Phase 2** (line 339) | "Phase 2: false-prune rate is 0 on the corpus (HCT + liveness only, no speculation)." | In the new model (§7.1 Step 4, §7.4), ERG moves states from `ACTIVE` to `CACHED` (`PLACEMENT_EVICT`) and from `CACHED` to `STALE` (`CACHE_CAPACITY`). It is undefined whether moving a state from `ACTIVE` to `CACHED` (or evicting it to `STALE`) counts as a "prune" for the Phase 2 false-prune rate metric. | [DECISION-PENDING] |
| **§8.2 Phase 3** (line 340) | "Phase 3: bound-violation rate is 0; speculation mismatch rate is measured and recorded." | Previously RSB was invoked only on missing dependencies with HCT pre-filtering candidates; in §7.1 Step 6–7 and §7.4, RSB is triggered by `BOUNDARY_VIOLATION` (`ACTIVE -> QUARANTINED`) while execution continues with the quarantined state's current contents until `OVERRIDE_APPLIED`. | [DECISION-PENDING] |
| **§8.2 Phase 4** (line 341) & old Invariant 3 | "Phase 4: relevance either reduces cost without increasing mismatch, or is removed." / old §7.3 "Relevance: orders candidates by hotness and history. Never removes." | In §7.1 Step 4 and §7.4 (`PLACEMENT_EVICT`), relevance score below threshold for `N` frames removes a state from the `ACTIVE` set and places it in `CACHED`. Relevance therefore changes `ACTIVE` set membership rather than only ordering RSB candidates, requiring hypothesis H7 to test that `CACHED` placement never alters frame hashes. | [DECISION-PENDING] |
| **§5.1 Threads** (lines 143–147) & **§5.2 Rules** (line 152) |Lists 5 threads (CPU, GPU frontend, Executor, Validator, Services) and forbids blocking/locks on the GPU frontend hot path. | Step 3 allows a dedicated state tracker thread ([DECISION-PENDING]), and Step 7–8 describes RSB returning asynchronously ("Execution continues with the quarantined state's current contents... When RSB returns, the override is recorded"), which implies an RSB worker thread or async queue not listed in §5.1. | [DECISION-PENDING] |
| **§6.4 Semantic IR versioning** (lines 177–178) | "Speculative nodes carry status: `pending`, `verified`, `invalid`." | §7.4 defines 5 state lifecycle states (`ACTIVE`, `CACHED`, `STALE`, `QUARANTINED`, `OVERRIDDEN`), §7.8 defines 3 frame statuses (`CLEAN`, `SPECULATIVE`, `RESOLVED_PENDING_RECORD`), and §7.9 defines 2 IR packet statuses (`FINAL`, `SPECULATIVE`). Mapping between §6.4's per-node `{pending, verified, invalid}` and §7.4/§7.8/§7.9 is unspecified. | [DECISION-PENDING] |

---

## 8. Testing

### 8.1 Test categories
- **Unit:** pure functions, data structures, allocators, handles. Run on every commit.
- **Property:** randomized inputs for the decoder, IR serialization round-trips, and HCT invariants. Seeds are logged for reproduction.
- **Golden:** fixed captures with expected frame hashes. Changes require an explicit golden update with justification in the commit.
- **Differential:** main path vs reference path on every test capture. This is the primary correctness gate.
- **Fault injection:** deliberately corrupt speculative nodes at a set rate; verify detection and recovery. Required before any claim about recovery behavior.
- **Benchmarks:** `tests/bench/`. Report p50, p99, and max frame time. Never report averages alone. Benchmarks run on a pinned machine profile recorded in the output.

### 8.2 Gates by phase
Mirrors the phase plan in the research design doc:

- **Phase 0:** replay of one deterministic capture produces identical hashes across 3 runs.
- **Phase 1:** validator detects 100% of injected frame corruptions.
- **Phase 2:** false-prune rate is 0 on the corpus (HCT + liveness only, no speculation).
- **Phase 3:** bound-violation rate is 0; speculation mismatch rate is measured and recorded.
- **Phase 4:** relevance either reduces cost without increasing mismatch, or is removed. Both outcomes are recorded.
- **Phase 5:** fault injection curves for time-to-detect, blast radius, and recovery cost are produced.

A phase is not complete until its gate is green in CI and the result is recorded in `docs/results/`.

### 8.3 Test rules
- Tests are hermetic: no network, no wall-clock dependency, no reliance on host GPU state beyond the declared device.
- Flaky tests are not retried silently. A flake is a bug; quarantine it with an issue number and a deadline.
- A bug fix includes a regression test that fails before the fix.

---

## 9. Scope and roadmap

**In scope now:** Maxwell (Switch 1) frontend, Vulkan executor, reference path, HCT for Maxwell, RSB/ERG experiments on test captures.

**Out of scope until the contract is frozen (after Phase 3):** Ampere frontend, Switch 2 support, commercial game compatibility, performance targets beyond the frame-time instrumentation.

**Contract freeze:** the Semantic IR contract is frozen after Phase 3 and verified against two frontends: the Maxwell frontend and a dummy frontend that emits synthetic IR. Only then does the Ampere frontend begin.

---

## 10. Review and contribution rules

- Every PR states: phase, invariants touched, tests added, and results (with links to CI artifacts).
- Design changes to RSB, ERG, HCT, or the IR require an RFC in `docs/rfcs/` before implementation.
- Architectural decisions are recorded as ADRs in `docs/decisions/` (context, decision, consequences, status).
- Reviewers check: memory ownership, hot-path allocations, concurrency boundaries, metaprogramming necessity, determinism, and whether the change weakens any invariant.
- A PR that weakens the reference path, adds unverified claims to docs, or introduces nondeterminism is rejected regardless of other merits.

---

## 11. Rules for automated agents

- Read this file and the relevant RFCs/ADRs before changing code.
- Do not mark a hypothesis as verified in documentation without a passing test ID.
- Do not delete failing tests to make a gate pass. Report the failure.
- Do not add dependencies without updating `THIRD_PARTY.md` and pinning them.
- Do not generate HCT entries from memory or inference alone. Entries must cite a source and a confidence level; if no source exists, mark `confidence = inferred` and do not use them for pruning.
- When uncertain about hardware behavior, stop and open an issue rather than encoding a guess into the HCT or the IR.
- Prefer the smallest change that satisfies the current phase gate.