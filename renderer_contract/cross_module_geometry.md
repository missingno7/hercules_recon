# ENG3 and TITLE polygon geometry paths

This audit traces one native polygon producer in each module to copied HERC
callbacks and its packet destination. It compares those routes with the
separately recorded ENG1 baseline; it does not use ENG1 as a proxy for either
module.

The candidate report has two independent proof records: `STATIC_OBSERVED` for
hash-pinned original PE spans and dataflow, and `BOUNDED_EQUIVALENT` for the
finite identity-matrix fixture against unchanged HERC callbacks. The fixture
does not execute any DLL producer. Its inputs are three signed WORD triplets
`(-20,10,0)`, `(10,20,0)`, `(30,-15,0)`, nine signed WORD matrix coefficients
with diagonal `4096`, translation `(0,0,1000,0)`, focal value 256, and center
`(320,240)`. It checks this sample, the camera DWORD lanes, screen WORD lanes,
FT4 destination order, and the separate one-point route. The observed screen
values are `(314,242)`, `(322,245)`, and `(327,236)`. This sample does not
distinguish general arithmetic order, low-WORD narrowing, or Z-clamp behavior.

## Callback contract

Both selected 3D FT4 producers use the same callback sequence for their first
three vertices and then project the fourth independently:

| Slot | Host RVA | Stack signature and proven value widths |
|---:|---:|---|
| 102 | `B430` | Three pointer arguments, caller-cleans 12 bytes. Each points to one vertex’s three signed WORDs `(X,Y,Z)`, 6 bytes per vertex. |
| 107 | `B320` | No arguments. The observed stores are `[ESI]` at `B367`, `[ESI+0x0C]` at `B3A3`, and `[ESI+0x18]` at `B3E1`; ESI starts at `A8E71C` and advances four bytes at `B403`. Thus X/Y/Z DWORD lanes start at `A8E71C`, `A8E728`, `A8E734`. Screen WORD stores use `[ECX+0x32]` at `B406` and `[ECX+0x36]` at `B422`; ECX starts at `A8E70E` and advances two bytes at `B414`, yielding X/Y lanes at `A8E740`/`A8E746`. |
| 117 | `B5C0` | One packet pointer, caller-cleans 4 bytes. Copies lane `i` to FT4 vertex `i`: `+08/+0A`, `+10/+12`, `+18/+1A`. |
| 101 | `B6F0` | One pointer to a three-signed-WORD XYZ point, caller-cleans 4 bytes. |
| 106 | `B920` | No arguments. Runs matrix transform and perspective projection; camera XYZ remain DWORDs and projected XY WORDs in host globals. |
| 114 | `B720` | One output pointer, caller-cleans 4 bytes. Writes two signed WORD screen coordinates (4 bytes). |
| 121 | `B870` | One output pointer, caller-cleans 4 bytes. Writes a WORD equal to camera Z arithmetic-shifted right by two. |

Static arithmetic is recorded separately for each callback. B740’s
single-point path accumulates the three signed products with low-32-bit
register wrap, arithmetic-shifts the row sum right 12, adds translation,
sign-extends the low WORD to a DWORD, and clamps Z to at least 1. B320’s batch
path independently shows 32-bit row accumulation, SAR 12 after the sum,
translation addition, and low-WORD sign extension into its camera DWORD lane;
the checked B320 span has no minimum-Z clamp and skips IDIV when Z is zero.
Those are static span observations, not a claim that the identity fixture
validates or generalizes B320 arithmetic from B740. Source coordinate units
remain unknown.

The module callsite and cleanup RVAs are pinned in the report. ENG3 uses
`0x23380/0x23385` for slots 102 and its corresponding slot-107/117/101/106/114
sites at `0x23388`, `0x233AA`, `0x233C1`, `0x233C9`, and `0x2340A`. TITLE uses
`0x183EC/0x183F1`, then `0x183F4`, `0x1841C`, `0x18433`, `0x1843B`, and
`0x18479`. The separate depth-gate callsites are `0x247BB/0x247C0` in ENG3
and `0x194BB/0x194C0` in TITLE. These callback-boundary layouts do not identify
mesh object types or coordinate units.

## Module routes

ENG3 handler-table entry 3 at `0x22E40` reaches projection/depth helper
`0x24770`, shared renderer `0x8DA0`, then conditionally calls the FT4 producer
`0x231A0` (callsite `0x901D`). That producer builds four local signed-WORD
triplets. Separately, its projection/depth gate loads three object DWORD fields,
arithmetic-shifts each right 16, then narrows to WORDs; their fixed-point units
are not established. The first three points pass through slots 102/107/117; the fourth passes
through 101/106/114 and writes XY at packet `+0x20/+0x22`. The packet cursor
is `0x10043164`, the stride is `0x28`, and the order-table head is reached
through current view state at `0x10041448`.

ENG3 loads nine matrix WORDs from global pointer `0x100394B8` through slot 108
(`B880`). It copies camera XYZ through slot 111 (`B2F0`) into context `+0x78`,
then slot 110 (`B8C0`) loads four translation DWORDs. In this sequence the
fourth source DWORD is not written by `B2F0`; its value is unresolved. Slots
174/175 exist, but this selected producer does not set projection center or
focal length. Their runtime setup remains external to this path.

TITLE mode-table index 5 reaches FT4 producer `0x181E0`. It independently
assembles three signed-WORD XYZ triplets plus a fourth point and uses the same
102/107/117 and 101/106/114 sequence. Its packet cursor is `0x1002DF40`,
stride `0x28`, with order-table state reached through current view
`0x1002CC04`. When global `0x1002D328` is nonzero, the writer calls `0x196F0`:
TITLE wrappers `0xCA40`/`0xCA60` invoke slots 164/165 (`BB00`/`BC30`), use the
HERC lookup table at RVA `0x37480` and TITLE’s nine-WORD matrix at `0x260E0`,
then slot 108 loads the result. The writer tail-calls wrapper `0xC790`, slot
103 (`B840`), to copy camera XYZ into translation. The source meaning of the
computed matrix is not established. Center/focal setup through slots 174/175
is not called by this producer.

The selected routes are separately compared with prior ENG1 evidence in
`geometry.json`. ENG1’s reviewed point emitters at `0x2E600`, `0x2E6C0`, and
`0x2ED10` call slots 101/106/114. Its `0x2DF20` producer directly calls the
GT4 packet callback and advances by `0x34`. Those are ENG1-specific findings;
this audit makes no inference that ENG3 or TITLE use the same source record or
the same culling behavior.

## Visibility and alternate producers

ENG3’s handler path marks a rejected parent with bit `0x08` at object `+0x1F`.
Its per-child gate accepts only when slot 121’s camera-Z word is greater than
`0x40`; the selected producer has additional state gates. TITLE’s separate
mode-3 path calls `0x19470` and applies the same `camera Z >> 2 > 0x40`
projection/depth gate before dynamic child dispatch. That does not establish
mode-5 packet visibility. The selected TITLE mode-5 path does not call slot 104
(`B4B0`); no polygon clipping behavior is inferred from the helper name
`0x196F0`.

Alternate 2D polygon writers bypass the 3D input/project/output route. ENG3
handler index 1 reaches `0x90E0`, an FT4 writer using existing WORD fields and
resource deltas. TITLE mode index 6 reaches dispatcher `0x03D10`, which selects
among `0x03420`, `0x03100`, `0x039B0`, and `0x03740`; `0x03420` writes
`0x28`-byte FT4 packets using signed WORD object positions plus resource byte
deltas. The checked producer spans contain no direct rel32 calls into the HERC
image that bypass the copied callback table.

## Limits and readiness

ENG3’s selected 3D route has a static source-to-packet association and a
bounded callback fixture, but the fourth translation DWORD and center/focal
setup remain external or unresolved. TITLE’s selected 3D route has a
conditional matrix seed and explicit camera snapshot, while center/focal
setup remains external; its separate depth gate is not proof of mode-5
visibility. ENG1 remains a prior baseline only. Other producer modes,
lighting/normal routes, full matrix families, and whole-module behavior are
outside scope.

The fixture checks only unchanged HERC callbacks with an identity matrix. It
does not claim that either DLL producer executed, that the fixture reproduces
game setup, or that output pixels are visible. No GPU, SDL, asset parsing,
gameplay, or original modification is involved.

## Candidate file map and reproduction

These private-lane paths mirror the intended package paths:

| Candidate | Intended package path |
|---|---|
| `renderer_contract/tests/audit_upstream_geometry.py` | `renderer_contract/tests/audit_upstream_geometry.py` |
| `work/renderer_phase14/cross_geometry/public_candidate/renderer_contract/evidence/upstream_geometry_modules.json` | `renderer_contract/evidence/upstream_geometry_modules.json` |
| `work/renderer_phase14/cross_geometry/public_candidate/renderer_contract/cross_module_geometry.md` | `renderer_contract/cross_module_geometry.md` |

The public runner accepts only a `.json` `--output` below the resolved and
lexical repository `work/` directory. It rejects parent traversal, redirected
work/output parents, the original `pc_install` tree, input-file equality, and
resolved destinations under `assets/`. The output guard checks every existing
directory component before and after creating the parent.

Tested from the repository root with the existing bundled Python runtime,
Capstone 5.0.7, and Unicorn 2.1.4:

```powershell
& 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' `
  renderer_contract/tests/audit_upstream_geometry.py `
  --repo-root 'D:\Prog\hercules_recon\work\renderer_interface_repo' `
  --exe 'D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE' `
  --eng1 'D:\Prog\hercules_recon\assets\pc\HERCULES\ENG1.DLL' `
  --eng3 'D:\Prog\hercules_recon\work\discs\pc_install\ENG3.DLL' `
  --title 'D:\Prog\hercules_recon\work\discs\pc_install\TITLE.DLL' `
  --output 'work/replay_upstream_geometry.json'
```

The checked input SHA-256 values are recorded in the candidate report. Guard
checks rejected a path under `assets/`, an output under `work/discs/pc_install`,
and a path containing `..`. The runner writes its output only under `work/`; the
candidate evidence JSON is a copy of that generated report for review.
