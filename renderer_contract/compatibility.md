# PC engine cross-module interface audit

## Finding

| Contract | ENG1 | ENG3 | TITLE |
|---|---|---|---|
| Cold interface copy | 250 DWORDs | 250 DWORDs | 250 DWORDs |
| Submit/init callbacks | +0x164 / +0x144 | +0x164 / +0x144 | +0x164 / +0x144 |
| OT view offset/count | +0x88 / 0x500 | +0x88 / 0x800 | +0x70 / 0x500 |
| Paired view stride | 0x1488 | 0x2088 | 0x1478 |
| Upload/move/color wrappers | +0x204 / +0x20C / +0x23C | Same separately observed slots | Same, plus +0x208 upload convenience |
| Projection center/focal | +0x2B8 / +0x2BC | Same separately observed slots | Same separately observed slots |
| Independent native producer evidence here | GT4/direct/observer/culled-prefix mesh and FT4 tail; variable sprite | Variable sprite plus direct `231A0` FT4 geometry association | `181E0` mode-5 FT4 geometry association in two resource contexts |
| Extra presentation route | Shared host presenter | Shared host presenter | +0x250 movie display |

The [notification target audit](notification_target.md) additionally verifies
the common slot-92 exchange ABI in all three modules. All five direct adapter
calls pass NULL. The [producer audit](producer_reachability.md) resolves TITLE
mode 7's computed command as FT4; consumer-only FT3 and fixed-sprite producer
extents remain unresolved. [Upload domain evidence](upload_domain_bounds.md)
ties TITLE's two local uploads to the original shared context and y=256, while
ENG3's script-derived split input remains resource-dependent.

Constructor-wrapper availability alone does not prove that a module emits
every corresponding packet. The indexed render handlers have bounded traces
in [eng3_handlers.md](eng3_handlers.md) and [title_handlers.md](title_handlers.md).
Coverage of other computed routes and TITLE's full
primitive inventory remain incomplete. Final host packet decoding is shared;
the independently measured frame counts differ.

The original `ENG1.DLL`, `ENG3.DLL`, and `TITLE.DLL` each export `PC_DLLEngineMain` by name at ordinal 1. Their entrypoints independently copy the same 250-DWORD / 1,000-byte interface prefix into different module-local storage roots, then validate the copied signature. Direct callback-cell references in each module confirm shared services and renderer paths, while TITLE has additional upload and movie-display paths that are absent from ENG1 and ENG3. The evidence is `STATIC_OBSERVED`: this audit reads pinned original PE bytes and disassembles them; it did not execute the originals or a gameplay session.

The copy contract and exact spans are:

| Module | Export RVA | Local 250-DWORD root | Initial copy gate | `rep movsd` RVA | Signature check RVA | Entrypoint span SHA-256 |
| --- | ---: | ---: | ---: | ---: | ---: | --- |
| ENG1 | `0x2c180` | `0x70f20` | `0x5b648` = 1 | `0x2c19f` | `0x2c1a1` | `597f7923e45606e27c452f9f52794e777d60d9f5a04119e9b0b8f08303f656c4` |
| ENG3 | `0x1f830` | `0x407a0` | `0x39488` = 1 | `0x1f84f` | `0x1f851` | `856441d73880be288bb62460b1332dcf56bdc544116fb85ec0004d2d71ecf510` |
| TITLE | `0x0c1f0` | `0x2bb40` | `0x257e8` = 1 | `0x0c20f` | `0x0c211` | `7f70098bc5be27c61bcd63df56c61b218f7884173466650025c3396b3b88048f` |

On each module's first valid non-null call, the entrypoint sets ECX to `0xfa` and copies 250 dwords before comparing the local word at offset zero with `0x5ac00cac`. After that, it loads `[arg1]` once and the loop tests that same captured dword while counting to 250; the loop does not validate each callback word. It then passes copied words at offsets `+0x28` and `+0x2c` to a module initializer and clears the per-module gate. A bad signature is rejected only after the local copy has already changed; the gate remains set for a later retry. On subsequent calls the gate is clear, so a new source-table pointer is not copied into that module. The later flags path masks arg2 with one and stores it at the copied context's `+0x40`. These details are separate observations for each binary, not inferences from ENG1.

The authoritative typed view remains [`calibration/engine_interface.h`](../calibration/engine_interface.h). The candidate helper below treats the interface only as a raw 250-word transport prefix and does not redeclare its typed fields.

## Host table and loader timing

HERCULES.EXE contains the 1,000-byte source table at RVA `0x361b8` / VA `0x4361b8`, in file-backed `.data`, with SHA-256 `52654fb050caa552b03c692bd3a1da0e700675bb50605394c8afb92595a039f8`. The adjacent pointer cell at RVA `0x365a0` / VA `0x4365a0` initially contains `0x004361b8`. Static code has one direct reference to that pointer cell: a read at RVA `0x5ef3` immediately before the export call at `0x5f03`; the caller cleans eight bytes at `0x5f05`. A separate direct-absolute scan finds reads of source-table data cells at slots 1, 2, 3, and 5 (186, 2, 2, and 2 sites respectively) and no direct code writes to any source-table cell. No direct references to the callback cells in this matrix were found. This supports a static file-backed initializer and no statically observed repopulation; a register-computed alias or runtime mutation is not excluded. The clone contract preserves all source data cells, including those direct host readers.

The host's generic module loader frees a prior handle, takes the basename after the final backslash from its mutable path buffer, calls `LoadLibraryA` at RVA `0x5c75`, then asks `GetProcAddress` at `0x5c95` for the exact name `PC_DLLEngineMain` and stores the result at VA `0x91c45c`. The host binary contains the relative strings `eng3\\release\\eng3.dll`, `eng1\\release\\eng1.dll`, and `title\\release\\title.dll`; the code passes a basename to `LoadLibraryA`, so these strings alone do not prove which DLL path or order is chosen at runtime. The three original modules import only `KERNEL32.dll`, with no static imports from one another.

Static host initialization calls graphics setup at `0x5e2c` before its later first engine dispatch. Graphics setup reaches the `DDRAW.dll!DirectDrawCreate` thunk at `0x7426`; the first source-pointer read and engine call occur at `0x5ef3` and `0x5f03`. Thus a data-only source-table clone published at `0x365a0` before the first cold engine call is consistent with the static call order. The pointer must remain valid for process lifetime. Changing the source pointer or source entries after a DLL's first successful initialization does not update that DLL's copied local table. Patching source entries before initialization is a global change shared by all subsequently initialized modules; redirecting `0x365a0` to a clone can keep all other entries intact and permits per-cell substitutions. Neither approach has been installed or tested in a live process.

## Callback map

The table below gives the source cell, host target, the modules with a direct copied-table call or jump, and the exact transfer instruction RVA. The `candidate` label means a plausible interception boundary; it is not a complete renderer allowlist. Ă˘â‚¬Ĺ›PreserveĂ˘â‚¬ĹĄ marks shared setup or projection services that a generic clone should normally copy unchanged.

| Interface slot / offset | Host source cell Ă˘â€ â€™ target RVA | Role supported by evidence | Direct module transfers | Clone policy |
| --- | --- | --- | --- | --- |
| 80 / `+0x140` | `0x362f8` Ă˘â€ â€™ `0xac00` | Frame/channel notification; AC00 calls BD80 and returns zero, with no head write or registration | ENG1 `0x2c5f4`; ENG3 `0x1fc04`; TITLE `0xc684` | Preserve |
| 81 / `+0x144` | `0x362fc` Ă˘â€ â€™ `0xac20` | 0x500-DWORD (0x1400-byte) frame-chain link initializer | ENG1 `0x2c60a`; ENG3 `0x1fc1a`; TITLE `0xc69a` | Preserve |
| 89 / `+0x164` | `0x3631c` Ă˘â€ â€™ `0xac60` | Frame submission / packet-chain consumer and software raster path | ENG1 `0x2c625`; ENG3 `0x1fc35`; TITLE `0xc6b5` | Candidate |
| 100 / `+0x190` | `0x36348` Ă˘â€ â€™ `0xb6b0` | Projected-coordinate pack/input service | ENG1 `0x2c67f`; ENG3 `0x1fc8f`; TITLE `0xc74f` | Preserve |
| 101 / `+0x194` | `0x3634c` Ă˘â€ â€™ `0xb6f0` | Geometry vector input | ENG1 `0x2c695`; ENG3 `0x1fca5`; TITLE `0xc765` | Preserve |
| 106 / `+0x1a8` | `0x36360` Ă˘â€ â€™ `0xb920` | Host transform and perspective operation | ENG1 `0x2c6e0`; ENG3 `0x1fd00`; TITLE `0xc7b0` | Preserve |
| 108 / `+0x1b0` | `0x36368` Ă˘â€ â€™ `0xb880` | Rotation matrix service | ENG1 `0x2c705`; ENG3 `0x1fd25`; TITLE `0xc7d5` | Preserve |
| 109 / `+0x1b4` | `0x3636c` Ă˘â€ â€™ `0xb8f0` | Translation service | ENG1 `0x2c715`; ENG3 `0x1fd35`; TITLE does not reference this slot | Preserve |
| 112 / `+0x1c0` | `0x36378` Ă˘â€ â€™ `0xb4a0` | Geometry service | ENG1 `0x2c745`; ENG3 `0x1fd65`; TITLE `0xc805` | Preserve |
| 114 / `+0x1c8` | `0x36380` Ă˘â€ â€™ `0xb720` | Geometry service | ENG1 `0x2c765`; ENG3 `0x1fd85`; TITLE `0xc815` | Preserve |
| 117 / `+0x1d4` | `0x3638c` Ă˘â€ â€™ `0xb5c0` | Geometry service | ENG1 `0x2c775`; ENG3 `0x1fd95`; TITLE `0xc825` | Preserve |
| 118 / `+0x1d8` | `0x36390` Ă˘â€ â€™ `0xb570` | Geometry service | ENG1 `0x2c785`; ENG3 `0x1fda5`; TITLE `0xc835` | Preserve |
| 119 / `+0x1dc` | `0x36394` Ă˘â€ â€™ `0xb660` | Geometry service | ENG1 `0x2c795`; ENG3 `0x1fdb5`; TITLE does not reference this slot | Preserve |
| 129 / `+0x204` | `0x363bc` Ă˘â€ â€™ `0xa710` | Texture rectangle upload into host-owned storage | ENG1 `0x2c7fa`; ENG3 `0x1fe3a`; TITLE `0xc89a` | Candidate |
| 130 / `+0x208` | `0x363c0` Ă˘â€ â€™ `0xa640` | TITLE seven-argument upload wrapper: arg1 is passed as source; A710's RECT16 low words are `{arg4,arg2,arg6,arg7}`; arg3 is unused in the body; packed return uses arg2/arg4/arg5 on the actual no-op A340 branch | TITLE `0xc8d3`; ENG1/ENG3 do not reference it | Candidate |
| 131 / `+0x20c` | `0x363c4` Ă˘â€ â€™ `0xa8f0` | VRAM rectangle move service; AC60 command `0x01` also calls A8F0 directly | ENG1 `0x2c81f`; ENG3 `0x1fe5f`; TITLE `0xc8ef`; AC60 direct call `0xb1e7` | Candidate |
| 143 / `+0x23c` | `0x363f4` Ă˘â€ â€™ `0x70c0` | Color-table translator | ENG1 `0x2c835`; ENG3 `0x1fe75`; TITLE `0xc965` | Candidate |
| 148 / `+0x250` | `0x36408` Ă˘â€ â€™ `0x5450` | TITLE movie-display service; separately locks, unlocks, and presents the host surface | TITLE `0xc9ea`; ENG1/ENG3 do not reference it | Candidate |
| 160 / `+0x280` | `0x36438` Ă˘â€ â€™ `0xa4a0` | 92-byte frame-header copy | ENG1 `0x2c8f5`; ENG3 `0x1ff35`; TITLE `0xca25` | Preserve |
| 174 / `+0x2b8` | `0x36470` Ă˘â€ â€™ `0xb970` | Projection center | ENG1 `0x2c9da`; ENG3 `0x1fffa`; TITLE `0xcb1a` | Preserve |
| 175 / `+0x2bc` | `0x36474` Ă˘â€ â€™ `0xb990` | Projection focal state | ENG1 `0x2c9f5`; ENG3 `0x20015`; TITLE `0xcb35` | Preserve |

The renderer paths include shared host calls that bypass their table cells. A710 invokes BD80 after upload on its `0xa7f7` and `0xa8db` paths. A640 directly calls A710 at `0xa675`, so replacing slot 129 alone misses TITLE's slot-130 upload route. AC60 command `0x01` calls A8F0 directly at `0xb1e7`; replacing slot 131 covers module calls through that copied cell but does not interpose this direct AC60 edge. AC60 also directly calls `0x70c0(0xff)` at `0xb2d0`, then unlocks at `0xb2d8` and presents at `0xb2dd`; replacing slot 143 alone does not intercept the AC60 color conversion. TITLE's movie callback at `0x5450` separately calls lock `0x6ff0`, unlock `0x7140`, and present `0x6620`, so intercepting slot 89 does not cover movie display. The direct-call edges are in this audit; the A710 slot role is also recorded in `evidence/eng1_pipeline.json`, and AC60/movie presentation is independently covered by `work/renderer_closure/closure.md` and `work/renderer_presentation/presentation.md`.

## Proxy feasibility and limits

A same-basename engine proxy is technically plausible because the host resolves `PC_DLLEngineMain` by name through `LoadLibraryA` / `GetProcAddress`; ordinal 1 is the original export ordinal. A proxy would need to forward the two physical stack arguments unchanged and preserve the original EAX result. The observed caller pushes the table pointer and a zero-extended flag byte, then performs caller cleanup of eight bytes; the host ignores the entrypoint result, but other use is unproved. Slot wrappers likewise carry per-callback argument and return behavior: in particular A640 returns a packed value. A forwarding DLL needs to reach the original module under another path or basename to avoid loading itself. Actual search-order resolution, selected path, module load order, and forwarding of all engine-specific behavior remain unverified.

The table-clone contract helper is private to `candidates/renderer_cross_module/`. It copies all 250 raw words and permits only the six candidate cells 89, 129, 130, 131, 143, and 148 plus the three explicitly documented shared-service cells 81, 174, and 175. Callers should normally leave the three shared-service cells untouched. Slots 130 and 148 cover the additional TITLE upload and movie paths; slot 131 covers module VRAM rectangle-move calls, while AC60 has the separately recorded direct call. The list is an evidence-backed bounded allowlist, not proof that every possible rendering callback has been found. Its table is a raw `renderer_u32` transport view from the shared width header, so offline x64 code preserves 32-bit words without treating them as native pointers. The helper does not load a process, install a hook, patch an image, or implement callback replacements.

Its synthetic VC5 x86 and MinGW-w64 x64 fixtures pass. Both check full preservation of every unselected word, selected-cell substitutions, source/output aliasing, invalid signature, out-of-range and unapproved slots, duplicates, excessive requests, and null arguments; every rejected request leaves output unchanged. The x64 build tests only raw offline words; it makes no live function-pointer width claim. Re-run both with:

See [README.md](README.md) for the shipped verification commands. Private audit commands are retained only in receipt provenance.

The pinned toolchain is `msvc5_rtm` (`C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953`), `/W3 /O2 /Gy /MT`. The portability check uses existing `C:\msys64\mingw64\bin\gcc.exe` (GCC 12.2.0, target `x86_64-w64-mingw32`) with strict C89 warnings. Test source hashes, commands, results, and executable hashes are in `table_clone_test.json`.

## Reproduction and evidence

Run the deterministic read-only audit from the repository root:

See [README.md](README.md) for the shipped verification commands. Private audit commands are retained only in receipt provenance.

It rechecks the four original-image SHA-256 values from `work/renderer_baseline/baseline.py`, then writes `evidence.json` with exact entry spans, module-local roots, initial flag bytes, direct table-cell call/jump rows, host source-cell xrefs, and the renderer slot matrix. Capstone is 5.0.7 and the PE reader is the repository's `scripts/pe.py`. Shared supporting files include `evidence/engine_interface_binding.json`, `evidence/engine_dispatch_contract.json`, `evidence/eng1_pipeline.json`, `docs/eng1_behavioral.md`, and the private renderer presentation / texture / frame reports.

Prediction: each of the three original engine mains would show an independent cold-copy gate, the same fixed count and signature, and host services bound through matching interface indices; the host table would be file-backed and passed immediately before engine initialization.

Falsifier: any module using a different count, signature, copy-before-check order, local root, or index-to-target mapping would disprove the shared contract for that module. A direct runtime write/repopulation of the host source table or a successful load before graphics setup would change the interposition-timing conclusion. No such contradiction was observed in the scanned PC images.

## Integrated reproduction

The private investigation paths above identify provenance. In this package, run tests/validate_originals.py with the original image paths to recheck recorded bytes and bindings, and tests/run_tests.py for the compiled synthetic suite. The byte validator does not independently redo the semantic analysis. The compact receipt is evidence/compatibility.json.

## Additional native evidence

Twelve DLL-prefix fixtures (three modules × valid cold, invalid signature,
null source and warm copy gate) observe all 250 copied words, selected overlay
retention, and each module's independent copy gate/table. Invalid signatures
are checked after copying. Fixtures stop before internal initialization calls
or at their bounded return; they do not prove full initialization.
See [native_interface_copy.json](evidence/native_interface_copy.json).

[module_packets.md](module_packets.md) adds separate ENG3 FT4/GT4 producer
routes and TITLE slot-118 FT4 construction. ENG3 also produces 12-byte 0x68
records; only their 8-byte link/tag prefix is consumer-proven. No producer
coverage is extrapolated from ENG1 to the other modules.

[Batch projection](batch_projection.md) verifies a zero-argument slot-107
observer in three finite original-x86 cases. The clone helper now accepts all
15 documented slots, including 101/102/106/107/114/117 for passive geometry
observation. Earlier receipts retain their historical allowlist scope.
[ENG3](eng3_geometry_association.md) and [TITLE](evidence/title_geometry_association.md)
now have independent original-producer point-to-corner fixtures; their upstream
visibility, complete producer coverage and runtime hooks remain unresolved.
