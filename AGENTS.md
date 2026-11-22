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
- **Policy-based templates for GPU frontends.** `template <class Gen> class GpuFrontend` where `Gen` satisfies a `GpuGeneration` concept (see §3.2). Maxwell and Ampere are instantiated, not virtually dispatched, in the hot path.
- **`constexpr` tables** for the Hardware Capability Table when the source data is static. Table validation (ranges, monotonicity, no duplicate keys) runs at compile time via `static_assert`.
- **Concepts** to constrain every template parameter. Unconstrained template parameters are rejected in review.
- **Code generation from data.** The HCT source of truth is `data/hct/*.toml`. A generator (`tools/hctgen`) produces `hct_generated.hpp` with a checked-in output and a CI check that the generated file matches the source. Generated files are never hand-edited.
- **`if constexpr`** for generation-specific branches inside a single template, instead of duplicating code.
- **Type-level state encoding** where it removes runtime checks, e.g., a `Recording` vs `Recorded` capture type so that replay cannot be called on an open capture.

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

### 7.1 Overview
```text
Guest image ─► Loader ─► Process & memory setup
                   │
   ┌───────────────┴───────────────┐
   ▼                               ▼
CPU (Dynarmic)                   Services (HLE)
   │                               │
   └──────► GPU command queue ◄────┘
                   │
                   ▼
          GPU frontend<Gen>        [Maxwell | Ampere]
                   │
                   ▼
            Capture writer
                   │
   ┌───────────────┴───────────────────┐
   │  Hardware Capability Table (HCT)  │   static, generated
   │  Context model                    │   per-resource state
   └───────────────┬───────────────────┘
                   ▼
     RSB (bounded hypothesis)      [experimental]
                   │
                   ▼
ERG: liveness → HCT bound → relevance (ordering only)
                   │
                   ▼
     Semantic IR (versioned contract)
                   │
   ┌───────────────┴───────────────┐
   ▼                               ▼
Vulkan executor                  Reference path
   │                               │
   ▼                               │
Frame output                       │
   └──────────► Validator ◄────────┘
                   │
    pass / mismatch → checkpoint & rollback
```

### 7.2 Module layout
```text
src/
  core/        assert, log, result, arena, pool, handles, hashing
  loader/      image parsing, memory layout
  cpu/         Dynarmic integration, interpreter fallback
  services/    HLE services (minimal, test-driven)
  gpu/
    frontend/  template <Gen> frontend, capture writer
    gen/       maxwell/, ampere/ (decode tables, command words)
    hct/       generated HCT, generator glue
  rsb/         bounded hypothesis generation, guards
  erg/         liveness, bound filter, relevance ordering
  ir/          semantic IR definitions, serialization, versioning
  exec/        executor interface, Vulkan backend (gfx/)
  validate/    reference path, hashing, comparison, metrics
  checkpoint/  snapshot and rollback
tools/
  hctgen/      HCT generator
  capdump/     capture inspection (read-only)
tests/
  unit/ integration/ golden/ fault/ bench/
data/
  hct/         TOML source of truth
docs/
  rfcs/ concurrency.md decisions/ (ADRs)
```

### 7.3 Components

**Loader, memory, CPU, services [standard].** Dynarmic is used as a dependency, not reimplemented. Interpreter first for validation; JIT enabled after the baseline is green. Services implemented on demand by test ROM need.

**GPU frontend [standard/hypothesis].** Templated on generation. Decodes command words into capture entries. Must be deterministic. Maxwell is implemented first and serves as the reference generation.

**Capture writer [hypothesis].** Serializes decoded operations, resource versions, ordering, and synchronization into the capture format (§6.1). The capture is the unit of replay and the ground truth source.

**Hardware Capability Table (HCT) [hypothesis].** Per-generation constraints: supported formats, binding limits, size limits, and impossible combinations. Each entry carries `source`, `confidence` (`documented`, `measured`, `inferred`), and `verified_by` (test ID). Enforcement rule: HCT removes only candidates that cannot exist on the target hardware. It never removes candidates merely because they are rare.

**Context model [hypothesis].** Per-resource state for each point in the capture:
- value (binding, format, dimensions)
- version (monotonic per resource)
- ordering (position relative to barriers and render-pass boundaries)
- synchronization state (last writer, pending reads, fences)
- dirty guest ranges

Context is what RSB uses to guess and what ERG uses to decide relevance of guesses. Omitting ordering and synchronization from the context model is a known source of false prunes and is forbidden.

**RSB (bounded hypothesis) [hypothesis, experimental].** Invoked only when a dependency is incomplete. Chooses one candidate from the set permitted by HCT and ERG.
- Candidate set is computed before selection; RSB does not generate candidates outside it.
- Every output is marked `speculative` with a unique `spec_id` and a `guard`: a predicate that must still hold for the node to remain valid (e.g., "resource R version == v at capture position p").
- Selection is deterministic given the seed in the run manifest.
- RSB has no direct access to the executor or Vulkan.

**ERG (three layered filters) [hypothesis].** Applied in fixed order:
1. **Liveness:** removes nodes proven dead (written and overwritten before any read, with no side effects). Only nodes with a proof are removed.
2. **Hardware bound:** removes RSB candidates that violate HCT. Only impossible candidates are removed.
3. **Relevance:** orders candidates by hotness and history. **Never removes.** Relevance can only affect order and therefore which candidate RSB prefers when multiple are equally valid.

Side-effecting operations (fences, semaphores, queries, CPU readbacks, presentation) are never removed by any filter.

**Freeze mode.** Relevance and hotness are frozen for a window of N frames (default 60, configurable) and then re-enabled. Hotness contributes only after a delay of M frames. This breaks the feedback cycle between RSB choices and the bounds that constrain them.

**Semantic IR [hypothesis, contract].** The only interface between frontend and executor. Contains no GPU-generation-specific detail. Versioned (§6.4). Speculative nodes are tracked by status and can be invalidated individually.

**Vulkan executor [standard].** Pure execution. Receives IR, looks up pipelines by semantic hash in the cache, compiles on miss, records and submits command buffers, and presents. Makes no semantic decisions. Pipeline and shader caches are required from the first working version.

**Reference path [standard].** Capture → executor, with no RSB, no ERG, no speculation. Produces the ground-truth frame hashes. It must be runnable in CI for every test capture.

**Validator [hypothesis].** Compares the main path's output hashes against the reference path, per frame and per region. Modes: `full` (default in early phases), `sampled` (later phases), `off` (release builds, only after zero mismatches are demonstrated on the test corpus).

**Checkpoint and rollback [standard/hypothesis].** Snapshots are taken at safe synchronization points (end of render pass, fence signal, readback). On mismatch: roll back to the last verified checkpoint, invalidate speculative nodes from that point, and replay the capture with verified state. Rollback cost is measured and reported; it is not assumed to be cheap.

**Confidence and backoff.** A mismatch lowers confidence for the resource class involved and switches that class to conservative mode (no speculation) for K frames. Confidence recovers gradually. This prevents oscillation between guessing and failing.

### 7.4 Invariants (must hold; violations are bugs)

1. Capture is a deterministic function of its input.
2. HCT never removes a candidate that is valid on the target hardware.
3. Relevance never removes a node.
4. Every speculative node is traceable to its `spec_id`, guard, and candidate set.
5. The executor makes no semantic decisions.
6. The reference path is independent of RSB, ERG, speculative caches, and freeze/relevance state.
7. Side-effecting operations are never removed.
8. IR nodes are immutable after emission, except for the status field of speculative nodes.
9. Frame output is a function of (capture, configuration, seed). Non-determinism is a test failure.

### 7.5 Context model: what is tracked

| Field | Tracked by | Used by |
|---|---|---|
| Resource value (format, dims, bindings) | Frontend | RSB, HCT |
| Resource version | Frontend | Guards, ERG liveness |
| Ordering vs barriers / render passes | Capture writer | RSB, ERG |
| Sync state (writer, readers, fences) | Capture writer | RSB, ERG, guards |
| Dirty guest ranges | Memory layer | RSB, checkpoint |
| Hotness / history | ERG | Relevance only (ordering) |

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