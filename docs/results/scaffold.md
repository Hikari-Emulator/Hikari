# Architecture Scaffolding and Comparator Results

## 1. End-to-End Subsystems and Status

| Module | Files | Status | Notes |
| --- | --- | --- | --- |
| `src/core/` | `assert.*`, `hash.*`, `id.hpp`, `result.hpp`, `spsc_queue.hpp` | Working | XXH3-128, SHA-256, `Result<T>`, `ErrorCode::kNotImplemented`, lock-free bounded `SpscQueue<T, Capacity>` |
| `src/capture/` | `format.hpp`, `writer.*`, `reader.*`, `fixture.*` | Working | Capture format v1 + synthetic fixtures A, B, C, D, E |
| `src/loader/` | `loader.hpp`, `loader.cpp` | Partly working / stubbed | `parse_synthetic_image` and `GuestMemory` (with `DirtyRange` epoch log) run in `integration.runtime_scaffold`; `load_commercial_nro` returns `ErrorCode::kNotImplemented` |
| `src/cpu/` | `cpu_backend.hpp`, `cpu_backend.cpp` | Stubbed | `CpuBackend` interface with `DynarmicBackend` and `InterpreterBackend`, both returning `ErrorCode::kNotImplemented` |
| `src/services/` | `service_host.hpp`, `service_host.cpp` | Partly working / stubbed | `MinimalServiceHost::query_clock_ticks` returns deterministic ticks; `open_filesystem`, `poll_input`, `submit_audio` return `ErrorCode::kNotImplemented` |
| `src/gpu/gen/` | `generations.hpp`, `generations.cpp` | Partly working / stubbed | `GpuGeneration` concept checked by `static_assert` on `MaxwellGen` (decodes command words) and `AmpereGen` (returns `ErrorCode::kNotImplemented`) |
| `src/gpu/hct/` | `hct.hpp`, `hct.cpp` | Working (inferred stub) | `check_boundary_specs` checks format and surface dimensions against generation limits |
| `src/gpu/frontend/` | `frontend.hpp`, `frontend.cpp` | Working | `GpuFrontend<Gen>`, `StateTracker`, and `emit_ir_packets_from_capture` |
| `src/rsb/` | `rsb.hpp`, `rsb.cpp` | Working | Deterministic bounded candidate resolver with `Guard` and `spec_id` |
| `src/erg/` | `erg.*`, `hct_stub.*`, `lifecycle.hpp`, `placement_policy.*` | Working | Fixed liveness pass (ADR 0003), relevance ordering, `constexpr` lifecycle transition table + `TypedState<S>`, replaceable `RelevancePlacementPolicy` |
| `src/ir/` | `ir_packet.hpp`, `ir_packet.cpp` | Working | Versioned immutable `IrPacket` (`kFinal` / `kSpeculative`) |
| `src/exec/` | `gfx/executor.*`, `specialized_selector.*` | Partly working / stubbed | `VulkanExecutor` consumes packets in `packet_id` order with pipeline semantic-hash cache; `specialized_selector` dispatches specialized template instantiations per state signature; `gfx::submit_to_vulkan_device` returns `ErrorCode::kNotImplemented` |
| `src/validate/` | `reference_replay.*`, `rsb_erg_replay.*`, `validator.hpp` | Working | Independent reference path, RSB+ERG replay, `Validator`, and `RunMetrics` |
| `src/checkpoint/` | `checkpoint.*`, `speculative_override.*`, `fault_recovery.*` | Working | `CheckpointManager` snapshots/overrides/rollbacks, speculative override convergence comparator, and fault injection harness |
| `src/runtime/` | `frame_loop.*`, `hikari_run_main.cpp` | Working | `FrameLoop` (single-threaded and threaded SPSC modes) and `hikari_run` CLI |

---

## 2. `hikari_run` Three-Run Verification

Commands:
```bash
./build/debug/tools/capdump/hikari_capdump --write-synthetic /tmp/scaffold_a.hkcp
./build/debug/src/runtime/hikari_run /tmp/scaffold_a.hkcp > /tmp/hikari_run_1.txt
./build/debug/src/runtime/hikari_run /tmp/scaffold_a.hkcp > /tmp/hikari_run_2.txt
./build/debug/src/runtime/hikari_run /tmp/scaffold_a.hkcp > /tmp/hikari_run_3.txt
diff -u /tmp/hikari_run_1.txt /tmp/hikari_run_2.txt
diff -u /tmp/hikari_run_1.txt /tmp/hikari_run_3.txt
```

Output (identical across runs 1, 2, and 3; `diff -u` empty):
```text
frame=0 main_hash=81ddf9f12d69f576db303a4a5e583832 reference_hash=81ddf9f12d69f576db303a4a5e583832 status=MATCH
frame=1 main_hash=7b09b468b267821fde6a85f9b4c1e8a5 reference_hash=7b09b468b267821fde6a85f9b4c1e8a5 status=MATCH
frame=2 main_hash=759e9e958993af3d594bd8f873755840 reference_hash=759e9e958993af3d594bd8f873755840 status=MATCH
```

---

## 3. Comparator Outputs

### a. Liveness Fix (`unit.rsb_erg` and `golden.rsb_erg_diff`)
```text
erg_liveness fixture=D frame0_before=3 frame0_removed=0 frame0_after=3
erg_liveness fixture=E frame0_before=3 frame0_removed=1 frame0_after=2 frame1_removed=0 frame2_removed=0
diff fixture=D_incomplete_dep frame=0 reference=ERROR(code=1) rsb_erg=c695c194052709417d5b47ab5ff1055b status=reference error, comparison not possible spec_candidates=1 pruned=0
```

### b. Single-Threaded vs. Threaded SPSC Mode (`integration.runtime_scaffold`)
```text
runtime_comparator frame=0 single=81ddf9f12d69f576db303a4a5e583832 threaded=81ddf9f12d69f576db303a4a5e583832 status=MATCH
runtime_comparator frame=1 single=7b09b468b267821fde6a85f9b4c1e8a5 threaded=7b09b468b267821fde6a85f9b4c1e8a5 status=MATCH
runtime_comparator frame=2 single=759e9e958993af3d594bd8f873755840 threaded=759e9e958993af3d594bd8f873755840 status=MATCH
```

### c. Specialized Pipeline Selection vs. Generic Path (`unit.specialized_selector`)
```text
specialized_comparator fixture=A_synthetic frame=0 signature=1 specialized=81ddf9f12d69f576db303a4a5e583832 generic=81ddf9f12d69f576db303a4a5e583832 status=MATCH
specialized_comparator fixture=A_synthetic frame=1 signature=1 specialized=7b09b468b267821fde6a85f9b4c1e8a5 generic=7b09b468b267821fde6a85f9b4c1e8a5 status=MATCH
specialized_comparator fixture=A_synthetic frame=2 signature=1 specialized=759e9e958993af3d594bd8f873755840 generic=759e9e958993af3d594bd8f873755840 status=MATCH
specialized_comparator fixture=D_incomplete_dep frame=0 signature=2 specialized=c695c194052709417d5b47ab5ff1055b generic=c695c194052709417d5b47ab5ff1055b status=MATCH
```

### d. Compile-Time Lifecycle Transition Table vs. Runtime Sweep (`unit.lifecycle_typestate`)
```text
lifecycle_typestate_test: total_pairs=45 legal=9 illegal=36 status=MATCH
```

### e. Adaptive Placement Policy On vs. Off (`golden.h7_relevance_placement_diff`)
```text
h7_placement fixture=A_synthetic frame=0 policy_off=81ddf9f12d69f576db303a4a5e583832 policy_on=81ddf9f12d69f576db303a4a5e583832 cached=1 reactivated=1 status=MATCH
h7_placement fixture=A_synthetic frame=1 policy_off=7b09b468b267821fde6a85f9b4c1e8a5 policy_on=7b09b468b267821fde6a85f9b4c1e8a5 cached=1 reactivated=1 status=MATCH
h7_placement fixture=A_synthetic frame=2 policy_off=759e9e958993af3d594bd8f873755840 policy_on=759e9e958993af3d594bd8f873755840 cached=2 reactivated=2 status=MATCH
```

### f. Speculative Override Convergence vs. Reference Path (`golden.speculative_override`)
```text
spec_override frame=0 status=2 speculative=81ddf9f12d69f576db303a4ae48866f2 reference=81ddf9f12d69f576db303a4a5e583832 differs=true
spec_override frame=1 status=3 speculative=7b09b468b267821fde6a85f9b4c1e8a5 reference=7b09b468b267821fde6a85f9b4c1e8a5 differs=false
spec_override frame=2 status=1 speculative=759e9e958993af3d594bd8f873755840 reference=759e9e958993af3d594bd8f873755840 differs=false
spec_override_summary quarantine_frame=0 override_frame=1 converged_frame=1 frames_until_convergence=1
```

### g. Self-Recovery Fault Injection (`fault.self_recovery`, `docs/results/fault-injection.csv`)
```csv
rate_percent,total_frames,injected_faults,detected_faults,rollbacks_completed,unrecovered_mismatches,time_to_detect_frames,blast_radius_regions,recovery_replayed_ops
0,100,0,0,0,0,0,0,0
1,100,1,1,1,0,1,2,3
5,100,5,5,5,0,1,2,15
20,100,20,20,20,0,1,2,60
```
