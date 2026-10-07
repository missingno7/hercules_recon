# Initial binary architecture

The PC images independently support the proposed platform/engine split.
The baseline EXE is the CD-installer copy extracted to
`work/discs/pc_install/HERCULES.EXE` (SHA-256
`587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`).
The installed EXE differs by one byte at file offset `0x94b0`; retain it as
variant evidence, not the original matching target. The three installed DLLs
equal the CD-installer copies. All original inputs remain immutable.
`HERCULES.EXE` imports DirectDrawCreate, DirectSoundCreate, WinMM timing, joystick
and MCI APIs, Windows UI/GDI, registry and shell services. The three game DLLs
import only KERNEL32 and each exposes one undecorated `PC_DLLEngineMain` export.
The DLLs access platform services through a caller-provided callback table.

| Module | `.text` virtual bytes | Export RVA | Initial responsibility |
| --- | ---: | --- | --- |
| HERCULES.EXE | 183,742 | none | Windows/MFC shell, platform APIs, archive/file handling, engine loader |
| ENG1.DLL | 306,908 | `0x2c180` | SRC1 hero/enemy/boss engine family |
| ENG3.DLL | 199,692 | `0x1f830` | SRC3 hero/enemy engine family |
| TITLE.DLL | 130,287 | `0xc1f0` | Menus, credits, passwords, tally/save flow, shared game utilities |

These are code-section sizes, not recovered function sizes or game-only code.
Static runtime code is included. PE metadata, hashes, resource leaves and full
imports/exports are in the evidence manifest. Reviewed provenance and exact
string locations are in `evidence/source_map.json`.

## Engine interface

TITLE's export at RVA `0xc1f0` copies 250 DWORDs from the first argument to its
local table at RVA `0x2bb40` on the initialization path. It checks the leading
magic `0x5ac00cac`; table offset 4 is a shared-state pointer. The adjacent code
starting at `0xc270` contains many argument-forwarding wrappers and indirect
tail jumps through table entries. For example, `0xc310` forwards one argument
through the slot at `0x2bb74`; `0xc7e0` forwards a pointer through `0x2bcf8`.
The cdecl stack cleanup and absolute slot references are directly visible.

Do not infer all callback signatures or names from adjacency. The export's
loop at `0xc21d` repeatedly tests one loaded DWORD, so a source reconstruction
must preserve that behavior rather than "fixing" it to validate the table.
The EXE references `PC_DLLEngineMain` at RVA `0x5c8f`, and contains release build
paths for `m:\pc\title`, `m:\pc\eng1`, and `m:\pc\eng3`.

## Runtime and language boundaries

All four PE optional headers report linker 5.0 and alignment 4096/512. They have
no CodeView/debug directory or COFF symbol table. DLLs retain relocation tables;
the EXE has none. Header timestamps describe builds on 20 August 1997 UTC, but
are evidence fields rather than independent historical authentication.

The EXE contains `AfxWnd42s`, `AfxControlBar42s`, `AfxMDIFrame42s`, and related
window-class strings, plus 39 `.?AV` RTTI class descriptors including `CWinApp`,
`CWnd`, `CDialog` and exception classes. Together with the absence of a dynamic
MFC import, this strongly identifies a static MFC 4.2-family component. MFC's
precise library build and members remain to be matched. This demonstrates C++
library code, not that every application translation unit was C++.

All modules contain Microsoft CRT error strings and kernel/TLS/heap support,
without an MSVCRT import. TITLE RVA `0x1d7b0` has the familiar CRT `rand` recurrence
`state = state * 214013 + 2531011`, using thread-local state. Runtime code should
be identified from historical libraries, not counted as recovered gameplay.
No comparable game-class RTTI names were found in the DLL data strings. That
does not prove they were compiled as C or that every C++ feature was disabled.

The EXE has 38 resource leaves: cursors, bitmaps, icons, menu/dialog/string tables
and grouped cursors/icons. The original language IDs include 1033 and 2057. None
of the four images has a VERSION resource; the DLLs have no resources at all.
No literal `.c`, `.cpp`, `.obj`, `.lib`, or `.pdb` provenance names were recovered
from their ordinary mapped string data in this pass. Windows SEH/CRT support
must not be confused with proof of application-level C++ exception use.

## Historical tree and console lineage

ENG1 preserves a substantial hierarchy under `P:\SRC1\HERO`, `ALIEN1`, `ALIEN2`
and `BOSS`: Hercules, Phil, Nessus, Hades, Hydra, Medusa and many smaller actor
names survive. ENG3 retains `P:\SRC3\HERO` and `ALIEN1` entries for Cyclops,
Torment, Souls, People and other entities. These strings lack source suffixes
and are also referenced by data tables, so they establish naming/hierarchy
evidence rather than literal C translation-unit filenames. Preserve them when
assigning semantic names; do not invent a complete historical file tree yet.

TITLE contains asset directories for menu, options, password, save game,
sequences, cheats and tally screens. Diagnostic strings identify `LinkedSoundFx`,
`RemoveFxLinks`, `RemoveFxIdLinks`, `ClearAllSoundFx_Excpt`, and
`LSGameMemoryCardLoop`. Their code xrefs survive even when logging calls target
an empty `ret` stub. The `RemoveFxLinks` function at `0x5a70` has been inspected:
it scans linked sound entries, conditionally calls a platform wrapper, and
clears matching links. This is stronger naming evidence than generic address
labels, without claiming an original source filename.

`ANIMPSX.BIN`, `map_psx.bin`, ISO-style `\MV\M26.;1` movie paths and the memory-card
name `bu00:B-sces-00891` survive in the PC images. Card-format/error diagnostics
are present in TITLE. These are direct evidence of retained console-era
interfaces/data conventions; PSX revision findings are documented separately.
They do not change the PC matching oracle or establish cross-platform function
identity by themselves.

## Pilot and evidence queries

TITLE is the smallest game code section and offers clean non-library functions
with strong call-site evidence. Early candidates cover doubly linked list
updates (`0x97b0`, `0x98a0`), a sentinel loop (`0xcee0`), packed color arithmetic
(`0x189a0`), decimal digit extraction (`0x1a2c0`), callback calls and switch flow.
Fresh VC5 `/O2` compilation reproduces the first four complete code contributions
including naturally emitted alignment padding. Current acceptance state,
sources and verification commands belong to `recovery.json`, not this document.
This result demonstrates a practical code-generation path, not whole-DLL layout
or exact compiler patch-level identification.

Regenerate the ignored SQLite index and request bounded context:

```powershell
python scripts/archaeology.py index
python scripts/archaeology.py strings 'P:\SRC1\BOSS'
python scripts/archaeology.py strings 'RemoveFx' --module TITLE.DLL
python scripts/archaeology.py context TITLE.DLL 0x5a70 0x60
python scripts/archaeology.py context TITLE.DLL 0xc1f0 0x80
```

The index contains mapped ASCII/UTF-16 strings, Capstone linear disassembly,
absolute/immediate reference leads and PE HIGHLOW pointer references. Every
query verifies the source-file hash. It deliberately requires an explicit range
and does not pretend linear disassembly proves function boundaries. Jump tables,
indirect targets, arbitrary data pointers in a non-relocatable EXE and embedded
data in code need further analysis. Databases and full disassemblies stay in
`work/`; only reviewed compact evidence is canonical.
