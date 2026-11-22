# Hikari

Hikari is a research project investigating bounded speculative semantic reconstruction for Nintendo Switch emulation, built around a versioned Semantic IR and an Execution Relevance Graph (ERG).

## Status

Hikari is an early-stage research prototype and cannot run games. Every architectural and performance claim in this repository is a **hypothesis** until a reproducible test in this repository demonstrates it. Measured results—including negative results—are recorded in `docs/results/`.

## Hypothesis

A hardware-derived bound (the Hardware Capability Table, HCT) combined with a proof-based liveness filter can keep speculative GPU state reconstruction safe, verified against a non-speculative reference path.

## Design

- **Semantic IR** — A versioned, GPU-generation-independent representation of decoded operations and their dependencies. Speculative nodes carry explicit `spec_id` and guard predicates.
- **RSB (Bounded Hypothesis)** — Generates speculative state candidates only within the set permitted by the HCT and ERG when a dependency is incomplete.
- **ERG (Execution Relevance Graph)** — Three ordered filters:
  1. *Liveness*: removes only operations proven dead (never side-effecting operations).
  2. *Hardware bound*: removes candidates that violate the HCT.
  3. *Relevance*: orders remaining candidates by hotness and history; **never removes** nodes.
- **Reference path** — A non-speculative path (capture → executor, bypassing RSB and ERG) used as the ground-truth oracle for differential validation.

## Goals

- Accurate, modular emulation with a verifiable, independent architecture
- Measured resource efficiency, with both positive and negative experimental results published
- Well-documented research with explicit invariants, RFCs, and ADRs

## Scope

- **Platform:** Desktop Vulkan only (no Android support).
- **Inputs:** Homebrew, original test ROMs, conformance suites, and user-owned captures only. No tooling for dumping, decrypting, or extracting commercial game data or keys.
- **Roadmap:** Maxwell (Switch 1) frontend and Vulkan executor first; Switch 2 and Ampere support are out of scope until the Semantic IR contract is frozen.

## Building

### Requirements

- CMake 3.25 or higher
- Clang 22.1.8 (tested on Linux x86_64; Clang 18 not tested) with C++23 support
- Vulkan SDK
- GLFW 3

### Commands

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

Available presets: `debug`, `release`, `relwithdebinfo`, `asan`, `ubsan`, `tsan`.

## Layout

See [`AGENTS.md`](AGENTS.md) §7.2 for the full module layout (`src/`, `tools/`, `tests/`, `data/`, `docs/`, `third_party/`).

## License

GPL-3.0. See [`LICENSE`](LICENSE) for details.
