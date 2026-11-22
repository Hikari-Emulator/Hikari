# ADR 0005: Dynarmic Toolchain Baseline and Build-Time Patches

Status: Proposed

## Context

`AGENTS.md` §1 specifies `Clang >= 18` as the primary compiler baseline. On the current Arch Linux development host, `clang++-18`, `clang++-19`, and `clang++-20` are not installed (and could not be installed automatically because `apt.llvm.org` / `llvm.sh 18` targets Debian/Ubuntu and `pacman` requires interactive `sudo` authentication). The available Clang binary on the host is `/usr/bin/clang++` (`clang version 22.1.8`).

Building the vendored `dynarmic` (`9d45823`) tree unmodified under Clang 22.1.8 fails in two places:
1. The `fmt` version bundled under `third_party/dynarmic/externals/fmt/` fails `consteval` evaluation in `FMT_STRING(...)` (`os.cc` and `format-inl.h`).
2. `third_party/dynarmic/externals/mcl/include/mcl/mp/typelist/lift_sequence.hpp` fails template-template argument deduction for `std::integer_sequence` under Clang 22.1.8.

## Decision

1. Keep `third_party/dynarmic/` unmodified in git.
2. Override `fmt::fmt` in `third_party/CMakeLists.txt` via `FetchContent` pinned to `fmtlib/fmt` `11.1.4` (`123913715afeb8a437e6388b4473fcc4753e1c9a`), preserving compile-time format string checking (`FMT_CONSTEVAL` is not disabled).
3. Copy `third_party/dynarmic/externals/mcl/` to the CMake binary directory at configure time and apply `third_party/patches/0001-mcl-lift-sequence-integer-sequence.patch` via `git apply`.
4. Scope `CMAKE_POLICY_VERSION_MINIMUM 3.5` strictly around `add_subdirectory(dynarmic)` and `unset()` it immediately after.

## Consequences

- `libdynarmic.a` and the full Hikari `debug` preset build and pass `ctest --preset debug` (12/12 tests) under Clang 22.1.8 without modifying tracked files under `third_party/dynarmic/`.
- **Baseline status:** Unresolved for exact Clang 18.x (`clang++-18` was not available to test on this host); this ADR remains `Proposed` until verified on a Clang 18 CI runner or workstation.
