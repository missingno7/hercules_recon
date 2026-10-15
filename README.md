# Hercules reconstruction

Reconstruct and understand the 1997 Windows **Disney's Hercules Action Game**,
then use that implementation as the foundation for a faithful modernized game.
Historical matching decompilation is the foundation. Behavioral reconstruction,
emulation, REA and rendering experiments contribute to the same body of knowledge.

The original PC binaries are the oracle; PlayStation builds provide secondary
evidence. No complete original EXE or DLL has been reproduced byte-for-byte,
and the complete source-built game is not playable. A preview, successful link
or finite behavioral comparison does not establish either result.

## Start here

- [Project direction and consolidation assessment](docs/project_direction.md):
  branch relationships, what to retain or retire, renderer alternatives and next work.
- [Working contract](AGENTS.md), [matching proof rules](docs/matching.md) and
  [reconstruction workflow](docs/workflow.md).
- [Original binary architecture](docs/architecture.md), [input inventory](docs/inventory.md)
  and [pinned tools](docs/toolchain.md).

`recovery.json` is the **sole function recovery authority**. `FUNCTION_MATCH`
requires fresh historical compilation and complete raw target-byte equality.
Other source, matching diagnostics and semantic evidence remain useful without
that status. Counts and historical reports are snapshots, never a second ledger.

## Current implementation

| Area | What is available | Main limit |
| --- | --- | --- |
| Historical reconstruction | Pinned VC5 tools, raw acceptance/regression gate, C/C++ source, data/library evidence and natural-link diagnostics | Source/data/library and link-layout closure remain incomplete |
| [TITLE](docs/title_behavioral.md) | Source-built module, reviewed entry inventory, native differentials and bounded prior host observations | No whole-module byte equality or exhaustive behavior proof |
| [ENG1](docs/eng1_behavioral.md) / ENG3 | Growing engine/actor/resource source and paired-PC comparisons | ENG1's genuine graph remains unresolved; ENG3 is partial source recovery |
| [Host](docs/host_runtime_reconstruction.md) | Loader, input, UI, timing, graphics, archive and sound source with original-code comparisons | Complete source-built host startup and game execution remain open |
| [Renderer contract](renderer_contract/README.md) | Original interface, packet/state/pixel/projection/resource evidence and portable references | Full writer ownership, reachable domains and upstream association remain open |
| [Modern experiments](dropin_renderer/README.md) / [REA](prototypes/rea_eval/README.md) | Synthetic-verified proxy/shim, indexed CPU/GPU diagnostics and optional analysis | Research/integration tools; no committed final deployment architecture |

These committed streams already occur in one ancestry. Branch names describe
work history, not independent projects. Dated investigations remain available
through these documents and the [historical handoff](docs/handoff.md).

## Build and verification

Use CPython 3.11+ and the existing pinned dependencies under `C:\tools`. On the
inspected Windows host, bundled Python works; the WindowsApps alias does not.

```powershell
$python = 'C:/Users/jiriv/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
$env:PYTHONPATH = 'C:/tools/hercules/python;C:/tools/gp-recon-python'
& $python scripts/match.py verify --report work/strict_verification.json
& $python -m unittest discover -s tests -p 'test_*.py'
```

In a secondary worktree, add `--original-root D:/Prog/hercules_recon` to strict
verification to read the primary checkout's pinned originals. Source and recovery
state still come from the calling worktree. The [workflow](docs/workflow.md)
documents source-graph and differential commands; modern builds live with their
subproject. Historical compilation can need approved execution outside the Windows
sandbox. Do not alter tools or flags to fix a launch failure.

`assets/` is immutable. `src/` and `calibration/` hold reconstruction and diagnostic
source; location alone does not grant acceptance. `evidence/` and `renderer_contract/`
hold durable findings. `scripts/` and `tests/` verify them. `dropin_renderer/` and
`prototypes/` isolate modern experiments. `work/`, `candidates/` and build products
are ignored. Never commit originals, extracted resources or tool installations.
