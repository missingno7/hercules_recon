# Hercules PC matching reconstruction

An evidence-driven reconstruction of the 1997 Windows **Disney's Hercules Action
Game**. The target is historical C/C++ compiled to original PC machine code.
PlayStation versions are secondary evidence; no port or modernization is in scope.

## Current findings

- The supplied CD image is a later repack. Its InstallShield `DATA/DATA32.Z`
  preserves a baseline EXE; the installed EXE has an owner-confirmed one-byte no-CD patch.
  All three installed engine DLLs equal their installer counterparts.
- The PC EXE owns the Windows/DirectX shell; engine DLLs receive a callback table.
  PSX discs have corresponding ENGINE1, ENGINE3 and TITLE overlays.
- All four PC images report linker 5.0. VC5 RTM and SP2 are pinned candidate
  toolchains. The EXE contains static MFC 4.2-family evidence.
- The pilot and its exact status are recorded only in [recovery.json](recovery.json).
  No whole DLL or executable has been reproduced yet.

## Working layout

`assets/` contains immutable local originals. `evidence/` contains compact measured
identities and findings; `src/` accepted reconstruction; `calibration/` reproducible
diagnostic cases; `scripts/` analysis and matching tools. `work/`, `build/` and
`candidates/` are disposable and ignored. Historical tools live under `C:\tools`.
Never add game files or compiler installations to Git.

Use CPython 3.11 or later; pinned analysis packages load from `C:\tools\hercules\python`.
On the current host the WindowsApps CPython 3.11.9 works (the initial host used 3.12):

```powershell
$python = 'C:\Users\jiriv\AppData\Local\Microsoft\WindowsApps\python.exe'
& $python scripts/inventory.py --verify
& $python scripts/archaeology.py index
& $python scripts/archaeology.py strings 'ENGINE' --module TITLE.DLL
& $python scripts/archaeology.py function title.unlink_12c
& $python scripts/match.py verify
```

Install pinned dependencies with `scripts/setup_analysis.ps1` and compiler trees
with `scripts/setup_toolchain.ps1`. See [toolchain evidence](docs/toolchain.md),
[disc extraction and PSX comparison](docs/psx.md), [binary inventory](docs/inventory.md),
[architecture/source map](docs/architecture.md), and [matching policy](docs/matching.md).
The [shared-engine continuation](docs/shared_code.md) records exact target and
frame motion, vector, counted-list and bounds routines, measured reuse across DLLs, and the
remaining ENG3 vector variant.
Extraction must precede indexing the baseline EXE. Inside the Codex sandbox the
historical compiler must run outside it, because Windows rejects its old DLL layout
there; compiler binaries are unchanged.

The [bounded macro experiment](docs/macro_experiment.md) compares five whole-file
reconstruction waves over an 18-function actor subsystem slice. Its candidate
and reproducible ledger are diagnostic; canonical recovery was not changed.

The follow-up [near-match refinery experiment](docs/refinery_experiment.md) tests
bounded compiler-guided C families and a fresh twelve-function blind region.
One blind tail became raw exact; the four calibration targets did not converge.
Six earlier macro matches subsequently passed the normal canonical promotion gate.

The [effect follow-up](docs/effect_followup.md) closes the three remaining effect
code-generation differences, promotes four raw matches, and measures the first
natural caller/callee link. Its remaining call displacement difference records a
real layout problem; relocation-only equality is still not accepted as a match.

The [current worker workflow](docs/workflow.md) uses Luna xhigh for normal bounded
matching and Sol high for unresolved explanations. Compact context is derived
from existing evidence and recovery state; it adds no routing ledger. The
[production continuation](docs/workflow_experiment.md) records its first real run.

The [release-dependency continuation](docs/release_closure.md) repairs the actor
cleanup semantics, traces the copied engine interface from the original EXE, and
accepts its actual empty callback. Three cleanup helpers retain a measured partial
reconstruction; natural code/data linkage is still open.

The [resource loading continuation](docs/resource_loading_progress.md) adds an
exact engine diagnostic and reconstructs metadata, readiness, callbacks and
buffer destruction. It preserves callback-visible reloads and identifies the
archive-backed host dependencies; whole-module equality remains open.
The [archive continuation](docs/archive_dependency_progress.md) measures thirteen
dependencies and links them in an isolated synthetic-file integration fixture.
The [file-service continuation](docs/file_services_progress.md) adds host services
and six typed engine bridges, preserving all accepted matches and exercising the
combined host/archive pipeline with historical stdio.
The [resource-rebind continuation](docs/resource_rebind_progress.md) exercises real
owner relocation through host compaction and engine rebuilds, and identifies
thirteen additional CRT dependencies without rewriting library code.
The [dispatcher continuation](docs/dispatcher_progress.md) accepts a real host
callback, reconstructs dispatcher/callback dependencies and tests their native
cross-module callback chain. Header dependencies now identify compiler context.

The [leaf harvest](docs/leaf_harvest.md) measures every relocation-free leaf the
raw gate can accept without a natural link, identifies 83 library contributions,
and raises accepted functions from 44 to 113. It also records a measured VC5
translation-unit context effect usable for TU recovery.

The [current handoff](docs/handoff.md) records the verified checkpoint, reproducible
diagnostics and the next bounded reconstruction work.

## References

[icytower_rerecon](https://github.com/missingno7/icytower_rerecon) and
[blood2_recon](https://github.com/missingno7/blood2_recon) inform the process:
immutable evidence, isolated experiments, reproducible builds, and strict
acceptance. Hercules-specific conclusions come from measured Hercules evidence.
See [research/provenance](docs/references.md) and the [agent contract](AGENTS.md).
