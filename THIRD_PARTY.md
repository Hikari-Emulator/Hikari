# Third-Party Dependencies

All third-party dependencies must be pinned by commit hash in `third_party/` or via CMake `FetchContent` with a hash check (`AGENTS.md` §1).

> **Dynarmic provenance note:** `https://github.com/yuzu-mirror/dynarmic.git` is an unofficial mirror (its own repository description states so). The upstream author's repository (`merryhime/dynarmic`) was no longer available at the time of this pin, so provenance is recorded as **"mirror; upstream not verified."**
>
> **Excluded from `third_party/dynarmic/`:** `tests/`, `docs/`, and `externals/catch/` (`Catch2`, Boost Software License - Version 1.0 - August 17th, 2003 in `externals/catch/LICENSE.txt`) — **excluded: test-only** (`externals/CMakeLists.txt` adds `catch` only under `if (DYNARMIC_TESTS)`).

| Name | Version or commit | License (as read from file) | Source | Purpose | Verification command |
| --- | --- | --- | --- | --- | --- |
| xxHash | `v0.8.3` (`e626a72bc2321cd320e953a0ccf1584cad60f363`) | BSD 2-Clause License (`third_party/xxhash/LICENSE`) | `https://github.com/Cyan4973/xxHash.git` | XXH3-128 hashing for semantic content / frame hashes (`AGENTS.md` §6.2) | `git ls-remote https://github.com/Cyan4973/xxHash.git refs/tags/v0.8.3` |
| PicoSHA2 | `161cb3fc4170fa7a3eca9e582cebd27cc4d1fe29` | The MIT License (MIT) (`third_party/picosha2/LICENSE`) | `https://github.com/okdshin/PicoSHA2.git` | SHA-256 hashing for capture body integrity (`AGENTS.md` §6.1, §6.2) | `git ls-remote https://github.com/okdshin/PicoSHA2.git HEAD` |
| dynarmic | `9d4582339990d4eae53f1dc7160686920fc2075c` (`2024-03-05`, CMake project version `6.7.0`) | 0BSD text (`Copyright (C) 2017 merryhime <git@mary.rs>`, `Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted...` in `third_party/dynarmic/LICENSE.txt`) | `https://github.com/yuzu-mirror/dynarmic.git` (mirror; upstream not verified) | ARMv8-A (A64/A32) dynamic recompiler | `git -C /tmp/dyn-pin show 9d4582339990d4eae53f1dc7160686920fc2075c:LICENSE.txt` |
| fmt (`externals/fmt`) | Subtree in dynarmic `9d4582339990d4eae53f1dc7160686920fc2075c` (tree `70538833f272767f928e7085db6c332bbcc3be63`) | MIT-style license with optional binary embedding exception (`Copyright (c) 2012 - present, Victor Zverovich and {fmt} contributors` in `third_party/dynarmic/externals/fmt/LICENSE.rst`) | `https://github.com/fmtlib/fmt.git` via dynarmic subtree (mirror; upstream not verified) | Formatting library required by dynarmic & mcl | `git -C /tmp/dyn-pin show 9d4582339990d4eae53f1dc7160686920fc2075c:externals/fmt/LICENSE.rst` |
| mcl (`externals/mcl`) | Subtree in dynarmic `9d4582339990d4eae53f1dc7160686920fc2075c` (tree `e6d97e810cc9042b3081387f93749b0f63e0d2ef`) | `MIT License` (`Copyright (c) 2022 merryhime` in `third_party/dynarmic/externals/mcl/LICENSE`) | `https://github.com/merryhime/mcl.git` via dynarmic subtree (mirror; upstream not verified) | C++ utility/metaprogramming support library for dynarmic | `git -C /tmp/dyn-pin show 9d4582339990d4eae53f1dc7160686920fc2075c:externals/mcl/LICENSE` |
| robin-map (`externals/robin-map`) | Subtree in dynarmic `9d4582339990d4eae53f1dc7160686920fc2075c` (tree `bdcc6b511a043934a869059acc192904cbe5d223`) | `MIT License` (`Copyright (c) 2017 Thibaut Goetghebuer-Planchon <tessil@gmx.com>` in `third_party/dynarmic/externals/robin-map/LICENSE`) | `https://github.com/Tessil/robin-map.git` via dynarmic subtree (mirror; upstream not verified) | Fast robin-hood hash table used in dynarmic IR/JIT | `git -C /tmp/dyn-pin show 9d4582339990d4eae53f1dc7160686920fc2075c:externals/robin-map/LICENSE` |
| xbyak (`externals/xbyak`) | Subtree in dynarmic `9d4582339990d4eae53f1dc7160686920fc2075c` (tree `aeb20b28b913b4fcb1d3f5b48eef04c4b6b63192`) | BSD 3-Clause License (`Copyright (c) 2007 MITSUNARI Shigeo` in `third_party/dynarmic/externals/xbyak/COPYRIGHT`) | `https://github.com/herumi/xbyak.git` via dynarmic subtree (mirror; upstream not verified) | x86_64 JIT assembler backend for dynarmic (`x86_64` host) | `git -C /tmp/dyn-pin show 9d4582339990d4eae53f1dc7160686920fc2075c:externals/xbyak/COPYRIGHT` |
| zycore (`externals/zycore`) | Subtree in dynarmic `9d4582339990d4eae53f1dc7160686920fc2075c` (tree `02951198fff575fa2f216111f3d9c99a7960f6f9`) | `The MIT License (MIT)` (`Copyright (c) 2018-2020 Florian Bernd`, `Copyright (c) 2018-2020 Joel Höner` in `third_party/dynarmic/externals/zycore/LICENSE`) | `https://github.com/zyantific/zycore-c.git` via dynarmic subtree (mirror; upstream not verified) | Core C support library required by Zydis | `git -C /tmp/dyn-pin show 9d4582339990d4eae53f1dc7160686920fc2075c:externals/zycore/LICENSE` |
| zydis (`externals/zydis`) | Subtree in dynarmic `9d4582339990d4eae53f1dc7160686920fc2075c` (tree `8e460786d99b8c9694f8b533675bc0d7326e3b4c`) | `The MIT License (MIT)` (`Copyright (c) 2014-2021 Florian Bernd`, `Copyright (c) 2014-2021 Joel Höner` in `third_party/dynarmic/externals/zydis/LICENSE`) | `https://github.com/zyantific/zydis.git` via dynarmic subtree (mirror; upstream not verified) | x86_64 disassembler used by dynarmic (`x86_64` host) | `git -C /tmp/dyn-pin show 9d4582339990d4eae53f1dc7160686920fc2075c:externals/zydis/LICENSE` |
| oaknut (`externals/oaknut`) | Subtree in dynarmic `9d4582339990d4eae53f1dc7160686920fc2075c` (tree `bfd0ee848b1e792dd6dca46808ab18065589380d`) | `MIT License` (`Copyright (c) 2022 merryhime <https://mary.rs>` in `third_party/dynarmic/externals/oaknut/LICENSE`) | `https://github.com/merryhime/oaknut.git` via dynarmic subtree (mirror; upstream not verified) | ARM64 JIT assembler backend for dynarmic (`arm64` host) | `git -C /tmp/dyn-pin show 9d4582339990d4eae53f1dc7160686920fc2075c:externals/oaknut/LICENSE` |
| biscuit (`externals/biscuit`) | Subtree in dynarmic `9d4582339990d4eae53f1dc7160686920fc2075c` (tree `1b57769095324c4bf30076fd170b9d4bb67de256`) | MIT License text (`Copyright 2021 Lioncash/Lioncache` in `third_party/dynarmic/externals/biscuit/LICENSE.md`) | `https://github.com/lioncash/biscuit.git` via dynarmic subtree (mirror; upstream not verified) | RISC-V JIT assembler backend for dynarmic (`riscv` host) | `git -C /tmp/dyn-pin show 9d4582339990d4eae53f1dc7160686920fc2075c:externals/biscuit/LICENSE.md` |
| fmt (FetchContent override) | `11.1.4` (`123913715afeb8a437e6388b4473fcc4753e1c9a`) | MIT-style license (`Copyright (c) 2012 - present, Victor Zverovich and {fmt} contributors` in `LICENSE`) | `https://github.com/fmtlib/fmt.git` | Replaces dynarmic's bundled `fmt` at configure time so `FMT_STRING` compile-time checks remain enabled under Clang 22.1.8 | `git -C /tmp/fmt-pin cat-file -e 123913715afeb8a437e6388b4473fcc4753e1c9a` |

## Dynarmic Build Integration Notes

- **Compiler tested:**
  ```text
  clang version 22.1.8
  Target: x86_64-pc-linux-gnu
  Thread model: posix
  InstalledDir: /usr/bin
  ```
- **Build command:**
  `cmake -S . -B build/dyn -G Ninja -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DCMAKE_BUILD_TYPE=Debug && cmake --build build/dyn --target dynarmic`
- **Scoped flags and options (`third_party/CMakeLists.txt` only):**
  - `DYNARMIC_TESTS=OFF`, `DYNARMIC_USE_BUNDLED_EXTERNALS=ON`, `DYNARMIC_FRONTENDS="A64"`, `DYNARMIC_WARNINGS_AS_ERRORS=OFF`, `BUILD_SHARED_LIBS=OFF`.
  - `FMT_TEST=OFF`, `FMT_DOC=OFF`, `FMT_INSTALL=ON`, `MCL_INSTALL=ON`.
  - `CMAKE_POLICY_VERSION_MINIMUM=3.5` set immediately before `add_subdirectory(dynarmic)` and `unset()` immediately after (required because `externals/robin-map/CMakeLists.txt` specifies `cmake_minimum_required(VERSION 3.1)`).
  - No `-w` or `-Wno-error` flags were needed or applied. `hikari_warnings` remains attached to all Hikari targets.
- **Build-time patches (`third_party/patches/`):**
  - `third_party/patches/0001-mcl-lift-sequence-integer-sequence.patch`: applied at configure time to a build-tree copy (`${CMAKE_CURRENT_BINARY_DIR}/mcl-src`) via `git apply`, leaving `third_party/dynarmic/externals/mcl/` untouched. Adds an explicit `lift_sequence_impl<std::integer_sequence<T, values...>>` partial specialization to `include/mcl/mp/typelist/lift_sequence.hpp` for Clang 22.1.8.
