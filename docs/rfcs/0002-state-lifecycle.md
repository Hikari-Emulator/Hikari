# RFC 0002: Semantic State Lifecycle and Presentation Policy

- **Status:** Draft [DECISION-PENDING]

## Problem

`AGENTS.md` §7.1–§7.11 defines a five-state lifecycle (`ACTIVE`, `CACHED`, `STALE`, `QUARANTINED`, `OVERRIDDEN`) for `SemanticState` entries managed by the Execution Relevance Graph (ERG) and Recursive Semantic Bootstrapper (RSB), together with frame-level speculation statuses (`CLEAN`, `SPECULATIVE`, `RESOLVED_PENDING_RECORD`). Several transition conditions, missing `(state, event)` pairs, override synchronization boundaries, and presentation policies remain open decisions (`[DECISION-PENDING]`) and unverified hypotheses (`[HYPOTHESIS]`).

---

## Proposal: Lifecycle State Machine

States: `ACTIVE`, `CACHED`, `STALE`, `QUARANTINED`, `OVERRIDDEN`. [DECISION-PENDING]

Trigger events (8 total): `PLACEMENT_EVICT`, `REQUEST_HIT`, `CACHE_CAPACITY`, `BOUNDARY_VIOLATION`, `BOUNDARY_VIOLATION_INFERRED`, `RSB_RESOLVED`, `RSB_FAILED`, `OVERRIDE_APPLIED`. [DECISION-PENDING]

| From | To | Trigger | Condition | Side effects | Tag |
|---|---|---|---|---|---|
| `ACTIVE` | `CACHED` | `PLACEMENT_EVICT` | relevance score below threshold for `N` consecutive frames; no pending speculation on the state | move to cache index; keep guard | [DECISION-PENDING] for `N` and threshold function |
| `CACHED` | `ACTIVE` | `REQUEST_HIT` | guard check passes | reuse cached content; increment use counter | [DECISION-PENDING] |
| `CACHED` | `STALE` | `REQUEST_HIT` | guard check fails | discard cached content; log reason | [DECISION-PENDING] |
| `CACHED` | `STALE` | `CACHE_CAPACITY` | cache over budget; selected by eviction policy | discard; log | [DECISION-PENDING] for eviction policy (LRU vs hotness-weighted LRU) |
| `ACTIVE` | `QUARANTINED` | `BOUNDARY_VIOLATION` | predicate of a documented or measured boundary is false | record boundary ID; submit RSB request | [DECISION-PENDING] |
| `ACTIVE` | `QUARANTINED` | `BOUNDARY_VIOLATION_INFERRED` | boundary `confidence = inferred` | record boundary ID or log only | [DECISION-PENDING]: inferred boundaries quarantine, or only log? |
| `QUARANTINED` | `OVERRIDDEN` | `RSB_RESOLVED` | candidate passes bound check | record override; schedule at synchronization boundary | [DECISION-PENDING] |
| `QUARANTINED` | `QUARANTINED` | `RSB_FAILED` | RSB returns no valid candidate | keep current contents; apply retry policy | [DECISION-PENDING]: retry count, and what happens after the final failure |
| `OVERRIDDEN` | `ACTIVE` | `OVERRIDE_APPLIED` | at synchronization boundary | replace contents; clear `speculation_id` | [DECISION-PENDING] |

Any `(from, event)` pair not in this table is an illegal transition. Debug builds assert (`HIKARI_ASSERT`). Release builds log and refuse the transition. [DECISION-PENDING]

### Missing pairs (documentation completeness listing of all 32 remaining `(state, event)` pairs out of `5 × 8 = 40` total combinations; this is a documentation check only and does not verify the design)

- **From `ACTIVE` (5 unlisted events):** [DECISION-PENDING]
  - `(ACTIVE, REQUEST_HIT)`: [DECISION-PENDING] (Option 1: no-op that increments hotness; Option 2: illegal transition event if `REQUEST_HIT` is emitted only for `CACHED` states).
  - `(ACTIVE, CACHE_CAPACITY)`: illegal transition (debug assert, release log and refuse). [DECISION-PENDING]
  - `(ACTIVE, RSB_RESOLVED)`: illegal transition (debug assert, release log and refuse). [DECISION-PENDING]
  - `(ACTIVE, RSB_FAILED)`: illegal transition (debug assert, release log and refuse). [DECISION-PENDING]
  - `(ACTIVE, OVERRIDE_APPLIED)`: illegal transition (debug assert, release log and refuse). [DECISION-PENDING]
- **From `CACHED` (6 unlisted events):** [DECISION-PENDING]
  - `(CACHED, PLACEMENT_EVICT)`: illegal transition (debug assert, release log and refuse). [DECISION-PENDING]
  - `(CACHED, BOUNDARY_VIOLATION)`: illegal transition (debug assert, release log and refuse). [DECISION-PENDING]
  - `(CACHED, BOUNDARY_VIOLATION_INFERRED)`: illegal transition (debug assert, release log and refuse). [DECISION-PENDING]
  - `(CACHED, RSB_RESOLVED)`: illegal transition (debug assert, release log and refuse). [DECISION-PENDING]
  - `(CACHED, RSB_FAILED)`: illegal transition (debug assert, release log and refuse). [DECISION-PENDING]
  - `(CACHED, OVERRIDE_APPLIED)`: illegal transition (debug assert, release log and refuse). [DECISION-PENDING]
- **From `STALE` (all 8 events: `PLACEMENT_EVICT`, `REQUEST_HIT`, `CACHE_CAPACITY`, `BOUNDARY_VIOLATION`, `BOUNDARY_VIOLATION_INFERRED`, `RSB_RESOLVED`, `RSB_FAILED`, `OVERRIDE_APPLIED`):** [DECISION-PENDING]
  - [DECISION-PENDING] (Option 1: `STALE` is terminal, all 8 events assert in debug and refuse in release, and slot reuse requires a new `StateId` generation; Option 2: allow re-initialization transition on a new write).
- **From `QUARANTINED` (6 unlisted events):** [DECISION-PENDING]
  - `(QUARANTINED, PLACEMENT_EVICT)`: illegal transition (debug assert, release log and refuse). [DECISION-PENDING]
  - `(QUARANTINED, REQUEST_HIT)`: [DECISION-PENDING] (Option 1: no lifecycle event fired while reading current quarantined contents; Option 2: self-transition `(QUARANTINED, REQUEST_HIT) -> QUARANTINED` recording speculative frame usage).
  - `(QUARANTINED, CACHE_CAPACITY)`: illegal transition (debug assert, release log and refuse). [DECISION-PENDING]
  - `(QUARANTINED, BOUNDARY_VIOLATION)`: [DECISION-PENDING] (Option 1: coalesce/ignore while `QUARANTINED`; Option 2: append `BoundaryId` to a per-state violation list).
  - `(QUARANTINED, BOUNDARY_VIOLATION_INFERRED)`: [DECISION-PENDING] (same options as `(QUARANTINED, BOUNDARY_VIOLATION)`).
  - `(QUARANTINED, OVERRIDE_APPLIED)`: illegal transition (debug assert, release log and refuse). [DECISION-PENDING]
- **From `OVERRIDDEN` (7 unlisted events):** [DECISION-PENDING]
  - `(OVERRIDDEN, PLACEMENT_EVICT)`: illegal transition (debug assert, release log and refuse). [DECISION-PENDING]
  - `(OVERRIDDEN, REQUEST_HIT)`: [DECISION-PENDING] (Option 1: read pre-override contents and mark frame `SPECULATIVE` until `OVERRIDE_APPLIED`; Option 2: force synchronization boundary if within the same command list).
  - `(OVERRIDDEN, CACHE_CAPACITY)`: illegal transition (debug assert, release log and refuse). [DECISION-PENDING]
  - `(OVERRIDDEN, BOUNDARY_VIOLATION)`: [DECISION-PENDING] (Option 1: illegal because boundary checks only run after `OVERRIDE_APPLIED` returns the state to `ACTIVE`; Option 2: transition back to `QUARANTINED`).
  - `(OVERRIDDEN, BOUNDARY_VIOLATION_INFERRED)`: [DECISION-PENDING] (same options as `(OVERRIDDEN, BOUNDARY_VIOLATION)`).
  - `(OVERRIDDEN, RSB_RESOLVED)`: illegal transition (debug assert, release log and refuse). [DECISION-PENDING]
  - `(OVERRIDDEN, RSB_FAILED)`: illegal transition (debug assert, release log and refuse). [DECISION-PENDING]

---

## Alternatives and Open questions (`[DECISION-PENDING]`)

1. **Placement threshold and window `N` (`ACTIVE -> CACHED` on `PLACEMENT_EVICT`):** What threshold function over relevance inputs and what consecutive frame count `N` trigger eviction to `CACHED`? [DECISION-PENDING]
2. **Cache capacity eviction policy (`CACHED -> STALE` on `CACHE_CAPACITY`):** Pure LRU vs hotness-weighted LRU when the cache exceeds budget? [DECISION-PENDING]
3. **Inferred boundary specs (`BOUNDARY_VIOLATION_INFERRED`):** Do boundary specs with `confidence = inferred` transition an `ACTIVE` state to `QUARANTINED`, or only log diagnostic counters until promoted to `measured`/`documented`? [DECISION-PENDING]
4. **RSB failure retry policy (`QUARANTINED -> QUARANTINED` on `RSB_FAILED`):** How many retries are attempted on `RSB_FAILED`, and what state transition or fallback occurs after the final failure? [DECISION-PENDING]
5. **Unlisted `(state, event)` pairs with non-obvious semantics:** How should `(ACTIVE, REQUEST_HIT)`, `STALE` (all 8 events), `(QUARANTINED, REQUEST_HIT)`, `(QUARANTINED, BOUNDARY_VIOLATION*)`, `(OVERRIDDEN, REQUEST_HIT)`, and `(OVERRIDDEN, BOUNDARY_VIOLATION*)` be handled? [DECISION-PENDING]
6. **Presentation policy for `SPECULATIVE` frames (`AGENTS.md` §7.8):** [DECISION-PENDING]
   - *Option 1:* Present `SPECULATIVE` frames with a marked region. Cost: visible regional artifact; measurable by region diff against the reference path. [DECISION-PENDING]
   - *Option 2:* Hold `SPECULATIVE` frames until resolution. Cost: added presentation latency, unbounded if RSB stalls; measurable by p99 frame time. [DECISION-PENDING]
   - (*Option 3:* Present `SPECULATIVE` frames without marking — rejected for research output because it hides the thing being measured (`docs/decisions/0001-hypothesis-framing.md`). [DECISION] (`docs/decisions/0001-hypothesis-framing.md`))
7. **Convergence window `K` in Hypothesis H2:** Within how many frames `K` after `OVERRIDE_APPLIED` must the frame hash converge to the reference path hash? [DECISION-PENDING]

---

## Hypotheses (`H1`–`H7`)

| ID | Statement | Falsifying Test ID | Metric | Pass Criterion | Tag |
|---|---|---|---|---|---|
| **H1** | "H1: reactivation of a `CACHED` state whose guard passes yields the same frame hash as a fresh resolve from the capture." | `tests/golden/h1_cached_reactivation_diff_test` | Per-frame and per-region `XXH3-128` hash mismatch count between cached-reactivated run and fresh-resolve reference run | `0` hash mismatches across all frames and regions in the test corpus | [HYPOTHESIS] |
| **H2** | "H2: after an override is applied at the synchronization boundary, the frame hash converges to the reference hash within `K` frames." (`K` is [DECISION-PENDING]) | `tests/fault/h2_override_convergence_test` | Number of frames after `frame_index_at_override` until per-frame hash equals reference path hash | Convergence frame delta $\le K$ for 100% of injected/quarantined overrides | [HYPOTHESIS] |
| **H3** | "H3: a documented boundary spec flags every state whose reference-path hash is wrong for that boundary." | `tests/fault/h3_boundary_coverage_test` | Count of states with a reference-path mismatch on a documented boundary where the boundary predicate evaluated to true (unflagged) | `0` unflagged states among states causing a boundary mismatch | [HYPOTHESIS] |
| **H4** | "H4: presenting `SPECULATIVE` frames costs less p99 frame time than holding them." (Unmeasured) | `tests/bench/h4_speculative_presentation_bench` | p99 frame time (ms) under present-marked policy vs hold-until-resolved policy on identical capture + seed | `p99(present_marked) < p99(hold_until_resolved)` on pinned benchmark profile | [HYPOTHESIS] |
| **H5** | "H5: guard checks are cheaper than re-resolving the state." (Unmeasured) | `tests/bench/h5_guard_check_bench` | p99 guard check time per request (ns) vs p99 fresh state decode/resolve time (ns) on the test corpus | `p99(guard_check) < p99(fresh_resolve)` on pinned benchmark profile | [HYPOTHESIS] |
| **H6** | "H6: re-recording invalidated command buffers costs less than a full frame re-record." (Unmeasured) | `tests/bench/h6_command_buffer_rerecord_bench` | p99 command-buffer re-record time per override (us) vs p99 full-frame command-buffer record time (us) | `p99(override_rerecord) < p99(full_frame_record)` on pinned benchmark profile | [HYPOTHESIS] |
| **H7** | "H7: relevance-driven `CACHED` placement does not change the frame hash of any frame." | `tests/golden/h7_relevance_placement_diff_test` | Differential frame-hash mismatch count between relevance-caching enabled and reference path | `0` frame-hash mismatches across the entire test corpus | [HYPOTHESIS] |

---

## Implementation Notes (From Exploratory Scaffolding)

- **Compile-time transition table and typestate (`src/erg/lifecycle.hpp`, tested by `unit.lifecycle_typestate` in `tests/unit/lifecycle_typestate_test.cpp`):**
  - Because `CACHED + REQUEST_HIT` has two conditional rows in the table above (guard pass $\to$ `ACTIVE`, guard fail $\to$ `STALE`), encoding the transition table as a purely compile-time `(state, event)` lookup required splitting `REQUEST_HIT` into two event enumerators (`kRequestHitPass` and `kRequestHitFail`), yielding $5 \times 9 = 45$ `(state, event)` pairs (`9` legal, `36` illegal):
    ```text
    lifecycle_typestate_test: total_pairs=45 legal=9 illegal=36 status=MATCH
    ```
  - Compile-time typestate guarantees (`TypedState<S>::transition<E>()`) hold for single-state transition chains and end at the homogeneous container boundary (`std::vector<ErgNode>`), where the compile-time tag `S` is erased into the runtime `LifecycleState` field.
- **Hypothesis H7 on synthetic fixtures A–E (`src/erg/placement_policy.cpp`, tested by `golden.h7_relevance_placement_diff` in `tests/golden/h7_relevance_placement_diff_test.cpp`):**
  - Comparing `AlwaysActivePlacementPolicy` (`policy_off`) against `ThresholdPlacementPolicy{15}` (`policy_on`) produced `0` frame-hash mismatches across all frames of synthetic fixtures A, B, C, D, and E (`status=MATCH`).
- **Hypothesis H2 on synthetic fixture A (`src/checkpoint/speculative_override.cpp`, tested by `golden.speculative_override` in `tests/golden/speculative_override_test.cpp`):**
  - When a state is quarantined on Frame 0 and the override is applied at the next-frame synchronization boundary (Frame 1), Frame 0's speculative hash differs from the reference hash (`differs=true`) and converges at Frame 1 (`frames_until_convergence=1`):
    ```text
    spec_override_summary quarantine_frame=0 override_frame=1 converged_frame=1 frames_until_convergence=1
    ```
