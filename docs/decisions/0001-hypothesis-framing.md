# ADR 0001: Hypothesis Framing

- **Status:** Accepted

## Context

Hikari explores speculative semantic reconstruction (RSB) and relevance-ordered filtering (ERG) for GPU emulation. Treating unverified architectural ideas as established facts or proven optimizations risks masking correctness bugs, false prunes, and negative performance results.

## Decision

All architectural and performance claims in this repository are documented and treated as **hypotheses** until verified by reproducible tests, differential checks, or benchmarks recorded in `docs/results/`.

## Consequences

- Documentation and `README.md` frame RSB, ERG, and HCT claims as testable hypotheses rather than proven capabilities.
- Every phase gate requires empirical evidence in `docs/results/` before a hypothesis can be marked verified.
- Negative results (e.g., if relevance ordering does not improve cost without increasing mismatches) are recorded and acted upon rather than omitted.
