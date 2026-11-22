# ADR 0002: Clang-Format Language Standard Setting

- **Status:** Accepted

## Context

`AGENTS.md` §1 sets the repository C++ language standard to C++23 (`-std=c++23`). When configuring `.clang-format` with `Standard: c++23`, `clang-format` 22.1.8 rejects the file with an error:

```text
$ clang-format --version
clang-format version 22.1.8
$ echo "int main() { return 0; }" | clang-format -style="{Language: Cpp, Standard: c++23}"
<command-line>:1:27: error: unknown enumerated scalar
{Language: Cpp, Standard: c++23}
                          ^~~~~
Error parsing -style: Invalid argument
```

In LLVM `clang-format`, the `Standard` option enumerates `c++03`, `c++11`, `c++14`, `c++17`, `c++20`, `Latest`, and `Auto`; it does not define a `c++23` scalar.

## Decision

Set `Standard: Latest` in `.clang-format` so that `clang-format` parses and formats C++23 syntax using the latest standard rules supported by the installed toolchain. `AGENTS.md` §1 does not need to change because it specifies the compiler language standard (`C++23`) and requires formatting with `.clang-format`, not the literal YAML scalar `c++23`.

## Consequences

- `clang-format --dry-run --Werror` succeeds across all repository headers and source files under `clang-format` 22.1.8.
- Formatting rules track the latest C++ standard supported by the active `clang-format` binary.
