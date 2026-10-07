# Hercules PC matching reconstruction

An evidence-driven reconstruction of the 1997 Windows **Disney's Hercules Action
Game**. The target is historical C/C++ compiled to original PC machine code.
PlayStation versions are secondary evidence; no port or modernization is in scope.

## Current findings

- The supplied CD image is a later repack. Its InstallShield `DATA/DATA32.Z`
  preserves a baseline EXE; the installed EXE has a one-byte modification.
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

Use Python 3.12 (the WindowsApps `python` alias may be unusable). On the initial host:

```powershell
$python = 'C:\Users\Jiri\AppData\Local\Programs\Python\Python312\python.exe'
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
Extraction must precede indexing the baseline EXE. Compiler invocation on this
host requires running outside the Codex sandbox because Windows rejects its old
DLL layout inside the sandbox; compiler binaries are unchanged.

## References

[icytower_rerecon](https://github.com/missingno7/icytower_rerecon) and
[blood2_recon](https://github.com/missingno7/blood2_recon) inform the process:
immutable evidence, isolated experiments, reproducible builds, and strict
acceptance. Hercules-specific conclusions come from measured Hercules evidence.
See [research/provenance](docs/references.md) and the [agent contract](AGENTS.md).
