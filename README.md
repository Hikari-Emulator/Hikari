# Hikari

**A Research-Oriented Yuzu-like Nintendo Switch Emulator**

> *Illuminating only what matters — Recursive semantic understanding for efficient Vulkan execution.*

---

## Philosophy

Hikari is not just another Switch emulator. It is an experimental architecture project that explores **semantic understanding**, **recursive definition** (inspired by how human language works), and **dynamic relevance pruning**.

Instead of caching everything like traditional emulators, Hikari only keeps **relevant states** and actively destroys irrelevant ones using the **Execution Relevance Graph (ERG)**.

### Core Innovations

- **Semantic IR** — Operations represented by meaning, not just raw opcodes
- **Recursive Semantic Bootstrapper (RSB)** — A meta-analysis engine that recursively defines and evolves concepts at runtime
- **Execution Relevance Graph (ERG)** — Graph-based system for intelligent resource lifetime management
- **Dynamic State Evolution** — Ability to create, replace, and merge states in real time
- **Pure Desktop Vulkan** — No Android support. Focused on performance and efficiency on desktop hardware (including older GPUs like Pascal)

---

## Goals

- Be **Yuzu-like** in spirit (accurate emulation, modularity, game preservation) but fully independent in architecture
- Push the boundaries of metaprogramming and semantic analysis in emulation
- Achieve better resource efficiency through intelligent relevance pruning
- Serve as a well-documented research project

---

## Current Status

**Very Early Stage (Research & Prototyping)**

- Core architecture is under active design
- Semantic IR, RSB, and ERG are in development
- Not yet capable of running games
- Current focus: Building the intelligent runtime system first

---

## Requirements:

- CMake 3.24 or higher
- C++23 compliant compiler (Clang 18+ recommended)
- Vulkan SDK
- GLFW 3

___

## Project Structure

- `hikari-runtime/` — Core intelligence (Semantic, RSB, ERG, Scheduler, Authority)
- `hikari-vulkan/` — Vulkan execution backend
- `hikari-gpu/` — GPU command processing & semantic capture
- `hikari-cpu/` — CPU emulation layer
- `research/` — Experiments, benchmarks, and graph visualizations
- `docs/` — Architecture and philosophy documentation

---
