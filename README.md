# Hercules reconstruction

Reconstruct Disney's 1997 Windows Hercules as a faithful native source game,
eventually using SDL3. The original PC binaries are the evidence oracle; PSX
builds are secondary evidence. Historical compiler matching is a foundation for
that work, alongside semantic, source-graph, host, and renderer discoveries.

The goal is one game implementation. First-party code from HERCULES.EXE, TITLE,
ENG1, and ENG3 must ultimately exist as source. The finished native runtime must
not depend on those original executable modules.

`main` is the canonical integrated reconstruction. Source recovery, strict
matching, renderer evidence and platform work advance that same source tree.
Task branches are temporary and are removed after integration.

## Start here

- [Project direction](docs/project_direction.md) explains the target and proof boundaries.
- [Working contract](AGENTS.md) gives repository-wide rules.
- [Source guide](src/README.md) explains the module layout and how to look up a function's status.
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

## Repository layout

| Path | Contents |
| --- | --- |
| `src/pc/`, `src/title/`, `src/eng1/`, `src/eng3/` | Current reconstructed source by original PC module |
| `src/shared/` | Shared engine logic, interfaces, layouts and data owners |
| `recovery.json` | Per-function matching authority; accepted and incomplete units share `src/` |
| `config/` | Active source selection, link plans and bounded analysis recipes |
| `scripts/`, `tests/` | Analysis/build/verification tools and their regression fixtures |
| `evidence/`, `docs/` | Proof inputs, provenance and documented findings |
| `renderer_contract/` | Recovered renderer interfaces and reference utilities |
| `prototypes/`, `dropin_renderer/` | Explicit host, renderer and reconstruction experiments |
| `work/`, `candidates/`, `build/` | Ignored generated output and disposable experiments |

The old `behavioral/` and `calibration/` source directories are retired. Matching
status belongs to each function's ledger row, not its directory. See the
[cleanup record](docs/source_layout.md) for old-path lookup and proof preservation.

## Everyday commands

Use the configured Python 3.11+ environment and the pinned dependencies under
C:\tools. Put generated artifacts in ignored work/ or candidates/ directories.
The RAW and RELOCATED acceptance regression command is:

    python scripts/match.py verify --report work/strict_verification.json

To inspect current source without treating filenames as proof tiers:

    python scripts/source_status.py
    python scripts/source_status.py actor_pair_animation.c

A secondary worktree may add --original-root D:/Prog/hercules_recon to read the
primary checkout's originals. The source and recovery state still come from the
calling worktree. See [workflow](docs/workflow.md) for candidate compilation,
promotion, and source-graph diagnostics.

assets/ is immutable. Never commit original game files, extracted resources,
tool installations, generated builds, databases, or large disassemblies.
