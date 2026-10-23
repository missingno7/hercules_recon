# Hercules reconstruction

Reconstruct Disney's 1997 Windows Hercules as a faithful native source game,
eventually using SDL3. The original PC binaries are the evidence oracle; PSX
builds are secondary evidence. Historical compiler matching is a foundation for
that work, alongside semantic, source-graph, host, and renderer discoveries.

The goal is one game implementation. First-party code from HERCULES.EXE, TITLE,
ENG1, and ENG3 must ultimately exist as source. The finished native runtime must
not depend on those original executable modules.

## Start here

- [Project direction](docs/project_direction.md) explains the target and proof boundaries.
- [Working contract](AGENTS.md) gives repository-wide rules.
- [Matching](docs/matching.md) defines accepted function evidence.
- [Workflow](docs/workflow.md) gives the short reconstruction loop.
- [Original binary architecture](docs/architecture.md), [source inventory](docs/inventory.md),
  and [toolchain](docs/toolchain.md) provide supporting context.

recovery.json remains the authoritative function matching state. Existing
FUNCTION_MATCH entries retain their strict raw-byte meaning; FUNCTION_MATCH_RELOCATED
accepts exact complete code with independently verified references. Source coverage,
semantic evidence, link closure, and runtime behavior are separate facts.

## Current evidence streams

| Area | Role |
| --- | --- |
| Historical C/C++ reconstruction | Exact function evidence with pinned VC5 tools and protected prior matches |
| TITLE, ENG1, ENG3, and host source | Recovers behavior and dependencies; incomplete source is identified as such |
| renderer_contract | Evidence-backed original interfaces and reference utilities |
| dropin_renderer and REA experiments | Bounded diagnostic work; neither defines the final game architecture |

Dated reports are historical measurements. Read current status from recovery.json
and the relevant module evidence; do not treat old counts as current state.

## Everyday commands

Use the configured Python 3.11+ environment and the pinned dependencies under
C:\tools. Put generated artifacts in ignored work/ or candidates/ directories.
The RAW and RELOCATED acceptance regression command is:

    python scripts/match.py verify --report work/strict_verification.json

A secondary worktree may add --original-root D:/Prog/hercules_recon to read the
primary checkout's originals. The source and recovery state still come from the
calling worktree. See [workflow](docs/workflow.md) for candidate compilation,
promotion, and source-graph diagnostics.

assets/ is immutable. Never commit original game files, extracted resources,
tool installations, generated builds, databases, or large disassemblies.