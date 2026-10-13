# Texture backing and alias map

## Finding

The ordinary upload/page address envelope is `[0x96E6E0, 0xA6E6E0)`, a 1 MiB interval that can be addressed as sixteen 64 KiB pages. AC60's alternate branch selects the fixed base `0xA6E6E0`; it does not retain the ordinary page offset. For byte UVs that gives the 64 KiB window `[0xA6E6E0, 0xA7E6E0)`, not sixteen alternate pages. These are address ranges, not allocation or exclusive-ownership claims.

Both ranges are inside the PE `.data` virtual tail. `.data` begins at VA `0x435000` (RVA `0x35000`), has virtual size `0x76FB28`, and only `0x9600` raw bytes. The candidate ranges are beyond its raw-data end at VA `0x43E600`, so the loader supplies zero-filled image memory there. The PE has no dedicated texture section, initializer, or allocation-size declaration for either interval. Runtime writers can populate it; the zero-fill layout alone does not show which pages are used.

## Read-safety map

| Region | What the original code establishes | Read-safety conclusion |
| --- | --- | --- |
| `0x96E6E0..0xA6E6E0` | A710/A8F0 normal-domain address arithmetic spans this interval for fully contained rectangles (`0<=x`, `x+width_words<=1024`, `0<=y`, `y+height_rows<=512`); AC60 ordinary page selection uses `0x96E6E0 + (((tpage&0x1f)>>1)<<16)`. | Plausible ordinary backing envelope. Static and bounded evidence do not establish that all 16 pages are initialized or exclusively owned. |
| `0xA6E6E0..0xA7E6E0` | AC60 sets `A8E6FC` from `(tpage&0x180)==0x180`; each alternate override stores fixed `0xA6E6E0` into `A909FC`, after the ordinary indexed-base calculation. | Alternate byte-UV page-0 address window. It is not a proved whole-sprite read bound: sprite clipping can advance U/V without byte masking. |
| `0xA8E6E0..0xA9E6E0` | This interval is alternate-page index 2. It contains `A8E6F4/F8/FC` page/blend/alternate state, `A8E918..A90918` reciprocal table, and rasterizer scratch including `A909FC`. The full `.text` scan found 1,497 explicit memory operands in this interval. | Unsafe to treat as texture backing or snapshot: it aliases live renderer state. AC60's alternate byte-UV selection does not select page 2, but the high-y upload branch can arithmetically reach it. |
| `0xA6E6E0..0xAFE6E0` | Static A710/A8F0 high-y branch (`y>=512`) yields `row=0x1000+(y&0x1ff)` and reaches alternate address pages 0 through 8 for `x<1024`. | The fixed ENG1/ENG3 `{0,512,128,512}` upload is now natively verified only over `[0xA6E6E0,0xA8E6E0)`. A static witness `(x=284,y=514,w=1,h=1)` addresses reciprocal cell `0xA8E918`; no observed fixed producer reaches it. General resource/context bounds and allocation ownership remain unresolved. |
| `0x004361B8..0x004365A0` | File-backed 250-entry callback table; cell `0x004365A0` contains its base `0x004361B8`. The corrected full-section scan records direct read candidates and no direct write candidates; no table entry points into either candidate bank. | Separate from texture backing. See the [loader audit](loader.md) for scan coverage and limits. Dynamic/runtime mutation and unresolved aliases are not ruled out. |

The reciprocal table is initialized by original RVA `0x72A0`, called directly from `0x5E2C`. It writes `R[0]=0` and `R[i]=floor(0x40000000/i)` for `1<=i<=2047`, occupying `[0xA8E918,0xA90918)` (8 KiB). This is wholly within alternate address page 2, beginning at page offset `0x238`. `A909FC` is also explicitly used as selected-page pointer/sprite-U state in that same page.

## Callbacks and renderer state that need to stay coherent

The callback source table at VA `0x4361B8` has SHA-256 `52654fb050caa552b03c692bd3a1da0e700675bb50605394c8afb92595a039f8`. Its adjacent pointer cell at `0x4365A0` hashes to `808fbb8b89f159778e8ec1322e9c7a53c570dfe17e6e565367f97a9bee2a0e3b` and points back to the source table. Relevant entries are slot 129 (`+0x204`) → A710, slot 130 (`+0x208`) → A640, slot 131 (`+0x20C`) → A8F0, and slot 143 (`+0x23C`) → 70C0. All 250 table entries classify outside the candidate VRAM spans (244 point into `.text`, 4 into other image sections, 2 outside the image).

If software rasterization bypasses the original pixel loops, the host-side operations still need coherent data and state: A710 uploads bytes into the ordinary map; A640 is a seven-argument wrapper that calls A710 and also returns a packed mode; A8F0 copies within the same address map and invokes BD80; and BD80's callback/gate cells at `0xA8E780/0xA8E788` must retain notification behavior. A8F0's finite overlap cases copy forward and smear right/down overlap, so they do not establish memmove semantics. AC60 page state in `A8E6F4/F8/FC` and selected pointer `A909FC` must match each packet. Slot 143 / 70C0 is a separate color and live-surface remap service, not a texture-bank upload.

Sprite clipping is a read-bound caveat: original setup can add `-x/-y` to U/V without masking. The first source row uses `selected_page+U+(V<<8)`; subsequent rows add `0x100` and mask only that row offset to 16 bits. Therefore the alternate page-0 byte-UV window is not a safe upper bound for clipped sprites. See `renderer_contract/evidence/sprite_geometry.json` and `renderer_contract/sprite_geometry.md`.

## Evidence and limits

[texture_ownership.json](evidence/texture_ownership.json) records the pinned EXE identity, PE sections, callback table values and aliases, exact code-span hashes, all direct-memory reference groups/counts, base-selection sites, and the overlap witness. The audit is a Capstone x86-32 linear sweep of `.text` with skipdata enabled; its explicit-operand scan does not resolve arbitrary runtime pointers, other modules, or external DLL writes. It scans every byte offset in file-backed non-`.text` sections for candidate DWORD values; matches are retained as numeric candidates, never assumed to be pointers from range membership alone.

Existing finite original-code receipts establish A710 cases `(5,224,7,3)`, `(127,478,3,2)`, and empty `(9,240,4,0)`, plus nine A8F0 cases with overlap/page crossing and one A710 source alias. They prove only the tested normal y domains. AC60 page-mask fixtures cover FT4, GT4, and E1 with tpage `0x80/0x100/0x180`; their low page selector is zero, so nonzero ordinary page selection is static-only. Sprite geometry fixtures establish the wider clipped U/V behavior. These proofs do not establish allocation ownership, full address-domain validity, initialization of all 16 ordinary pages, or whole-renderer equivalence. The initial prediction/falsifier and its correction are in `prediction.json`.

Reproduce the static audit from the repository root:

```powershell
& 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' `
  renderer_contract\tests\audit_texture_ownership.py `
  --original-root D:\Prog\hercules_recon\work\renderer_interface_repo `
  --exe D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE `
  --output D:\Prog\hercules_recon\work\renderer_interface_repo\work\renderer_phase4\texture_ownership\evidence.json
```
