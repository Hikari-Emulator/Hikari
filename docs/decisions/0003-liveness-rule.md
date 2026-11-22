# ADR 0003: ERG Liveness Pruning Rule

- **Status:** Accepted

## Context

In the initial experimental ERG liveness pass (`src/erg/erg.cpp`), `is_overwritten_after` treated any subsequent operation targeting the same `ResourceId` as a full overwrite of the prior state. On Fixture D (`build_incomplete_dependency_fixture`), `ClearOp` wrote `color_target = {1, 1}` and was followed by `SampleOp(target = {1, 1}, sampled = {3, 1})`. Because `SampleOp` targeted `{1, 1}` without an explicit read dependency edge on `{1, 1}`, ERG falsely pruned `ClearOp({1, 1})`.

## Decision

A state node $S_1$ on resource $R$ is proven dead and removable by `Erg::run_liveness_pass()` if and only if:
1. $S_1$ itself has no side effect (`S1.has_side_effect == false`),
2. There exists a subsequent state node $S_2$ on the same resource $R$ (`S2.producer_op_index > S1.producer_op_index`) such that:
   - $S_2$ overwrites $R$ in full (`S2.overwrites_in_full == true`),
   - $S_2$ has no side effects (`S2.has_side_effect == false`), and
   - $S_2$ does not read or blend with the target resource (`S2.reads_target == false`), and
3. No operation reads $S_1$ at or before $S_2$'s producer operation index.

## Consequences

- `ClearOp` (full-surface clear with `overwrites_in_full = true`, `has_side_effect = false`, `reads_target = false`) kills an immediately preceding unread `ClearOp` on the same target (Fixture E Frame 0: `pruned = 1`).
- Partial or target-blending draw operations (`DrawPatternOp`, `SampleOp`, with `overwrites_in_full = false` or `reads_target = true`) never falsely kill a preceding `ClearOp` on the target surface (Fixture D Frame 0: `pruned = 0`).
