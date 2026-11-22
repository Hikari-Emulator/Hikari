# Dynarmic P1 and P2 Predictions (Committed Before Execution)

Date: 2026-11-22
Branch: `research/rsb-erg`
Toolchain: `clang version 22.1.8` (`x86_64-pc-linux-gnu`)

## Hash Stream Definition

Per basic block executed, serialize in fixed little-endian order:
1. Guest PC (`uint64_t`)
2. Retired instruction count in the block (`uint64_t`)
3. `X0..X30` (`31 x uint64_t`)
4. `SP` (`uint64_t`)
5. `NZCV` (`uint32_t`)
6. Written guest memory ranges: `count` (`uint64_t`), followed by `count` pairs of `(start: uint64_t, length: uint64_t)`

Hash the serialized byte buffer with XXH3-128 (`src/core/hash.hpp`). This is a semantic state hash, not a pixel hash.

---

## P1 (Straight-Line)

Initial state: `PC = 0x0000`, `X3 = 0x1000`, all other GPRs `0`, `SP = 0`, `NZCV = 0`, guest memory zeroed.

| Offset | Encoding | Hand Disassembly |
| --- | --- | --- |
| `0x0000` | `0xD28000A0` | `MOVZ X0, #5` (`sf=1, opc=10, hw=00, imm16=5, Rd=0`) |
| `0x0004` | `0xD28000E1` | `MOVZ X1, #7` (`sf=1, opc=10, hw=00, imm16=7, Rd=1`) |
| `0x0008` | `0x8B010002` | `ADD X2, X0, X1` (`sf=1, op=0, S=0, shift=00, Rm=1, imm6=0, Rn=0, Rd=2`) |
| `0x000C` | `0xF9000062` | `STR X2, [X3]` (`size=11, opc=00, imm12=0, Rn=3, Rt=2`) |
| `0x0010` | `0xD4000001` | `SVC #0` (`imm16=0`) |

### Predicted Final State (P1)
- `X0 = 5`, `X1 = 7`, `X2 = 12`, `X3 = 0x1000`
- 8-byte little-endian word at guest address `0x1000`: `12`
- Basic block count: `1` (single straight-line block ending at `SVC #0`)
- Retired instruction count: `5` (counting `SVC #0` as retired when it executes and traps to halt)

---

## P2 (10-Iteration Loop)

Initial state: `PC = 0x0000`, all GPRs `0`, `SP = 0`, `NZCV = 0`, guest memory zeroed.

| Offset | Encoding | Hand Disassembly |
| --- | --- | --- |
| `0x0000` | `0xD2800140` | `MOVZ X0, #10` (`sf=1, opc=10, hw=00, imm16=10, Rd=0`) |
| `0x0004` | `0xD2800001` | `MOVZ X1, #0` (`sf=1, opc=10, hw=00, imm16=0, Rd=1`) |
| `0x0008` | `0x8B000021` | `ADD X1, X1, X0` (`loop`: `sf=1, Rm=0, Rn=1, Rd=1`) |
| `0x000C` | `0xD1000400` | `SUB X0, X0, #1` (`sf=1, op=1, S=0, sh=0, imm12=1, Rn=0, Rd=0`) |
| `0x0010` | `0xB5FFFFC0` | `CBNZ X0, #-8` (`sf=1, op=1, imm19=0x7FFFE (-2 words = -8 bytes -> 0x0008), Rt=0`) |
| `0x0014` | `0xD4000001` | `SVC #0` (`imm16=0`) |

### Predicted Final State (P2)
- `X1 = 55` (`10 + 9 + 8 + 7 + 6 + 5 + 4 + 3 + 2 + 1`)
- `X0 = 0`
- Retired instruction count: `33` (`2` initial `MOVZ` setup instructions + `10 * 3 = 30` loop body/branch instructions + `1` `SVC #0`)
- Basic block count: `11`:
  - Block 0 at `0x0000`: `MOVZ`, `MOVZ`, `ADD`, `SUB`, `CBNZ` (5 instructions, taken to `0x0008`)
  - Blocks 1..9 at `0x0008`: `ADD`, `SUB`, `CBNZ` (3 instructions each, 9 iterations)
  - Block 10 at `0x0014`: `SVC #0` (1 instruction, after `CBNZ` falls through when `X0 == 0`)
