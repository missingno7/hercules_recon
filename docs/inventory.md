# Evidence and binary inventory

All 12 supplied files are hashed in `evidence/manifest.json`, including the full
PC/PSX BIN images and CUE files. Original assets remain untouched. Disc extraction
and all generated reports are ignored. The manifest records every MZ executable
found in the installed directory and extracted PC disc, including NE-format
16-bit support binaries; complete primary-module PE metadata includes imports,
exports, resources, sections, debug directories and symbol-table absence.

## Matching targets

| Module | Bytes | PE timestamp (UTC) | Image base | Code virtual bytes |
|---|---:|---|---|---:|
| HERCULES.EXE (installer baseline) | 275,968 | 1997-08-20 14:23:22 | 0x00400000 | 183,742 |
| ENG1.DLL | 481,280 | 1997-08-20 14:21:43 | 0x10000000 | 306,908 |
| ENG3.DLL | 272,896 | 1997-08-20 14:18:21 | 0x10000000 | 199,692 |
| TITLE.DLL | 180,736 | 1997-08-20 14:19:45 | 0x10000000 | 130,287 |

Timestamps are header evidence, not an independently authenticated build date.
All report x86 PE32, linker 5.0, section alignment 4096 and file alignment 512.
The DLLs have `.text`, `.rdata`, `.data`, `.idata`, `.reloc`; the EXE has `.rsrc`
instead of `.reloc`. Each DLL exports only `PC_DLLEngineMain`.

The authoritative EXE for matching is extracted from `DATA/DATA32.Z` into
`work/discs/pc_install/HERCULES.EXE`, SHA-256:

`587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`

It differs from the supplied installed EXE at exactly file offset `0x94b0`
(VA `0x0040a0b0`): baseline byte `53` (`push ebx`) becomes `c3` (`ret`). The
installed EXE equals the disc's `NOCD` and `WIN_X64` copies. This establishes a
modification; an independently authenticated original retail disc remains useful
for further provenance. No binary was patched to establish the target.

All three installer DLLs are identical to the installed DLLs. Their immutable
`assets/pc/HERCULES/` copies are the DLL matching targets. `HERCULES.FS` is a game
data archive, not PE code, and equals its installer copy. The HLP files differ.

## Disc contents and roles

The PC CD image is a later repack: volume creation fields point to 2020-12-27,
and added `WIN_X64`, `NOCD`, and `CORRECT/INSTALL` directories corroborate this.
`DATA/DATA32.Z` is the historical InstallShield package; extraction is documented
in `docs/psx.md` and does not execute an installer.

`SETUP`, `_SETUP`, `_ISDEL`, `_ISRES`, `ISDBGN`, `UNINST16`, `AUTOPLAY`, and
`CTL3D` belong to installation/launch UI. `REDIST/DIRECTX` holds DirectX runtimes,
setup programs, display/audio drivers and their multilingual variants. These are
dependency/library evidence, not game-source reconstruction targets.
`CORRECT/INSTALL` is added support software of separate provenance.

Each PSX disc contains a small boot executable and three PS-X EXE engine/title
overlays; these are secondary evidence only. See `evidence/psx.json` and
`docs/psx.md` for header identities, whole-image hashes and revision differences.

## Regeneration

After extraction, `scripts/inventory.py` writes the manifest if absent. It refuses
to replace an existing evidence baseline. `--verify` rehashes all supplied files,
checks sizes, and detects newly added or missing original files. Derived file
metadata can be regenerated into scratch by importing `create_manifest()`.
