# Dynarmic P1 and P2 Results Against Interpreter Oracle

Date: 2026-11-22
Branch: `research/rsb-erg`
Toolchain: `clang version 22.1.8` (`x86_64-pc-linux-gnu`)
Submodule: `third_party/dynarmic` at `5373c127ff58449b1488a649c21686b597312917`
Predictions file: [dynarmic-predictions.md](dynarmic-predictions.md) (`6570c25`)

## Summary

- `golden.cpu_oracle` passed under `debug`, `asan`, and `ubsan` presets (`14/14` tests passed in all three presets).
- `switch1_a57` and `switch2_a78c` produced identical per-block hashes and final register/memory state for both P1 and P2.
- **Wrong predictions:** None. All predicted register values, memory values, block counts, and instruction counts in `6570c25` matched actual execution on both `InterpreterBackend` and `DynarmicBackend`.

## P1 (Straight-Line)

| Metric | Predicted | `switch1_a57` (Dynarmic & Interpreter) | `switch2_a78c` (Dynarmic & Interpreter) |
| --- | --- | --- | --- |
| `X0` | `5` | `5` | `5` |
| `X1` | `7` | `7` | `7` |
| `X2` | `12` | `12` | `12` |
| `X3` | `0x1000` | `0x1000` | `0x1000` |
| `SP` | `0` | `0` | `0` |
| `NZCV` | `0` | `0` | `0` |
| `mem[0x1000]` (u64) | `12` | `12` | `12` |
| Block count | `1` | `1` | `1` |
| Retired instructions | `5` | `5` | `5` |
| Hash status | — | `MATCH` (`8172a031b473cca4e80b8361afaeca16`) | `MATCH` (`8172a031b473cca4e80b8361afaeca16`) |

## P2 (10-Iteration Loop)

| Metric | Predicted | `switch1_a57` (Dynarmic & Interpreter) | `switch2_a78c` (Dynarmic & Interpreter) |
| --- | --- | --- | --- |
| `X0` | `0` | `0` | `0` |
| `X1` | `55` | `55` | `55` |
| `SP` | `0` | `0` | `0` |
| `NZCV` | `0` | `0` | `0` |
| `mem[0x1000]` (u64) | `0` | `0` | `0` |
| Block count | `11` | `11` | `11` |
| Retired instructions | `33` | `33` | `33` |
| Hash status | — | `11/11` `MATCH` (last block `795fc87f15517d9f990ddc4a7e08ce5c`) | `11/11` `MATCH` (last block `795fc87f15517d9f990ddc4a7e08ce5c`) |

## Not Verified

- Only Clang 22.1.8 (`x86_64-pc-linux-gnu`) tested (Clang 18 not tested).
- Only hand-assembled P1 and P2 executed.
- No SVC handling beyond exit (`SVC #0`).
- No fastmem or page-table fast path (all guest memory accesses routed through `UserCallbacks`).
- `[HYPOTHESIS]` CPU profiles (`switch1_a57`, `switch2_a78c`) are not validated against Switch 1 / Switch 2 hardware.
- No GPU connection.
