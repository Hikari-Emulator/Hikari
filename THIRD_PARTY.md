# Third-Party Dependencies

All third-party dependencies must be pinned by commit hash in `third_party/` or via CMake `FetchContent` with a hash check (`AGENTS.md` §1).

| Name | Version or commit | License | Purpose |
| --- | --- | --- | --- |
| xxHash | `v0.8.3` (`e626a72bc2321cd320e953a0ccf1584cad60f363`) | BSD-2-Clause | XXH3-128 hashing for semantic content / frame hashes (`AGENTS.md` §6.2) |
| PicoSHA2 | `161cb3fc4170fa7a3eca9e582cebd27cc4d1fe29` | MIT | SHA-256 hashing for capture body integrity (`AGENTS.md` §6.1, §6.2) |
