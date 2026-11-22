# ADR 0004: Contributor policy for AI-assisted and human-written code

Status: Accepted (draft by project owner; review before merge)

## Context

Hikari is a research emulator with claims that must be tested. Code quality and
honesty of documentation matter more than who or what produced a change. The
project has seen LLM-generated output that is plausible but unverified, and
descriptions that claim more than the evidence shows.

## Decision

1. Authorship does not matter. Human-written and AI-assisted code are held to
   the same standard. No contributor is treated as a lesser author for using AI.

2. AI-generated "slop" is not accepted. A change is rejected if it:
   - adds code the author cannot explain line by line,
   - adds abstraction, metaprogramming, or indirection without a comparator
     that can show it is wrong (AGENTS.md §0, §3),
   - violates the architecture in AGENTS.md §7 (for example, lets the executor
     make semantic decisions, or gives RSB a path to Vulkan),
   - adds dead code, placeholder logic that returns plausible values, or stubs
     that do not return `kNotImplemented`,
   - adds docs or comments that state unverified results as facts.

3. The change must run and be maintainable. Before a change is ready for
   review, the author shows:
   - the build and test commands run, with output,
   - the comparator results for any mechanism the change touches,
   - that the change is documented where the next maintainer needs it.

4. PR descriptions must not contradict the evidence. Specifically:
   - every claim in the description has a command or test behind it,
   - "not verified", "not run", and "stubbed" are stated where they apply,
   - a failing test is reported as failing, not omitted,
   - the description is checked against the diff and the output before submission.

5. Disclosure of AI assistance is optional for the code itself, but any
   LLM-generated analysis or result that a PR cites must be re-run by the
   author, and the command output must be in the PR, not the model's summary.

6. Reviewers may reject a change for any of the above regardless of who wrote it.

## Consequences

- Reviewers check evidence and architecture fit, not authorship.
- The project avoids a split between "trusted" and "untrusted" contributors.
- Overconfident documentation is treated as a defect, the same as broken code.

## Related

- AGENTS.md §0 (ground rules), §3 (metaprogramming), §7 (architecture), §10 (review).
- ADR 0001 (hypothesis framing).
