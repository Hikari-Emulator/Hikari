# ADR 0005: Dynarmic Toolchain Baseline and Fork Submodule

Status: Accepted

## Context

The Hikari development host runs Arch Linux (`x86_64-pc-linux-gnu`) with `clang version 22.1.8` (`/usr/bin/clang`, `/usr/bin/clang++`). Clang 18 is not installed on this host and is not tested.

Building the upstream `yuzu-mirror/dynarmic` pin (`9d4582339990d4eae53f1dc7160686920fc2075c`) unmodified under Clang 22.1.8 fails in `externals/robin-map/CMakeLists.txt` (CMake `< 3.5`), `externals/fmt` (`consteval` error in `os.cc:172`), `externals/mcl/include/mcl/mp/typelist/lift_sequence.hpp` (`std::integer_sequence` specialization), and `externals/mcl/include/mcl/macro/anonymous_variable.hpp` (`-Werror,-Wc2y-extensions` on `__COUNTER__`).

## Decision

- **Tested toolchain:** Clang 22.1.8 (`x86_64-pc-linux-gnu`). Clang 18 is not tested and the Clang 18 requirement is dropped.
- **Dynarmic integration:** `third_party/dynarmic` is pinned as a git submodule to `git@github.com:Hikari-Emulator/dynarmic.git` branch `hikari/clang22-build` (`5373c127ff58449b1488a649c21686b597312917`), branched from `base/9d45823` (`9d4582339990d4eae53f1dc7160686920fc2075c`), with the four Clang 22.1.8 / CMake 4.x fixes committed in the fork.
- **Hikari CMake isolation:** `third_party/CMakeLists.txt` sets `DYNARMIC_TESTS=OFF`, `DYNARMIC_USE_BUNDLED_EXTERNALS=ON`, `DYNARMIC_FRONTENDS="A64"`, and `DYNARMIC_WARNINGS_AS_ERRORS=OFF`, and calls `add_subdirectory(dynarmic)` without `-w`, `-Wno-error`, `FMT_CONSTEVAL=`, or `CMAKE_POLICY_VERSION_MINIMUM`.

## Consequences

- `libdynarmic.a` and the full Hikari `debug` preset build and pass `ctest --preset debug` (12/12 tests) under Clang 22.1.8 (`x86_64-pc-linux-gnu`).
- Only Clang 22.1.8 has been tested; no guest ARM64 program has run on `DynarmicBackend` yet.
