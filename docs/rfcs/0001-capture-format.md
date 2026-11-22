# RFC 0001: Capture Format

- **Status:** Draft

## Problem

Hikari requires a deterministic, self-contained binary capture format (`v1`) to serve as the unit of replay and ground-truth comparison between the speculative path and the non-speculative reference path (`AGENTS.md` §6.1, §7.3). Without a defined capture format, Phase 0 determinism tests and differential validation cannot be implemented.

## Open questions

1. How should command-word records and variable-length payloads be framed and aligned within the capture body?
2. Which exact metadata fields are required in the capture header beyond magic, format version, generation ID, frame count, and body SHA-256?
3. How should guest memory dirty ranges and synchronization state (writers, readers, fences) be encoded alongside decoded GPU operations?
4. Should captures support streaming/chunked writing for large sessions without violating the whole-body SHA-256 integrity check?
5. What versioning and forward-compatibility rules apply to `v1` records before the Semantic IR contract is frozen?
