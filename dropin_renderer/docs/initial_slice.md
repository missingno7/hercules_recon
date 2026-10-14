# Initial implementation checkpoint — 2026-10-14

The buildable modern subproject now consumes the original contract directly.
The working contract authorizes this isolated scope without changing historical
acceptance. `recovery.json`, `src/`, `calibration/` and assets have no changes in
this checkpoint. Work uses the existing `codex/renderer-interface-closure` checkout.

## Measured results

| Boundary | Fresh result | Limit |
|---|---|---|
| CMake x86 product and fixtures | All seven CTest cases pass, including four Python validation tests | GCC 16.2.0 on this Windows host; alternate MSVC recipe untested |
| Production proxy | PE32/i386; sole `DirectDrawCreate` export; imports only kernel32/msvcrt | No automatic original table publication or deployment claim |
| Source-built mock forwarder/adapter | Arguments, null/failure HRESULTs, unwrapped COM lifetime, clone invariants, callback return/error, recursion and concurrency pass | Synthetic callbacks and supplied table/pointer cell |
| Real system API | Standalone proxy loads system DirectDraw, returns S_OK/non-null object, Release returns zero; canonical path resolves to SysWOW64 | No Hercules, engine, cooperative-level, window or display operation |
| Shared contract | 22 VC5 suites and 22 strict C89/C++98 GCC x86 suites pass | Original-backed finite models retain their stated domains |
| Archived ENG1 source pipeline | Fresh original resource/preparation trace; 51 source preparation checks and 914 source-fed emission checks pass | Three controlled resource cases; supported zero-angle/uniform-scale services |
| Modern mesh consumer | Eight captured camera/screen pairs and twelve packet corners in three FT4 faces validate; deterministic 1920x1440 JSON produced | Original archived mesh, controlled camera/row; no live visibility or frame |
| Optional SDL GPU | D3D12 device, runtime SM5.1 shaders, owned vertex upload, offscreen draw and fence-waited RGBA8 readback pass | Solid-color diagnostic, with synthetic default 36,864 colored pixels; no texture or original-raster equivalence |
| Actual archived face on GPU | Validated coordinates submitted; all corners outside viewport, zero colored pixels | Clipping/readback evidence only, no visible game image |
| Historical strict regression | All 132 prior accepted functions remain raw-byte exact after fresh pinned compilation | No new function, module layout or whole-image acceptance |
| Original image audit | Four image identities, 702 spans and prior contract evidence pass; two new compact proof references appended | Static identities do not prove source semantics |

The new original-code investigation specifically discriminates command-01's
direct access envelope from its called routine's read span. With only 16 readable
bytes, A8F0 faults on source Y at offset 18; with 22 bytes it faults on height at
22; with 24 it returns through the controlled zero-height path and reloads the
live link. The shared C capture-size helper makes that knowledge useful to the
modern consumer while preserving previous decoder minima and statuses.

No new historical function implementation or `FUNCTION_MATCH` promotion is
claimed. The point/face and projection providers were reused, not reconstructed
again. The modern replay also checks that recorded dynamic offsets resolve to
the pinned archive's actual point/face DWORDs and rejects inconsistent joins.

## Reproduce and inspect

Commands are in [the subproject README](../README.md). The compact summary with
source, toolchain and report hashes is [initial_slice.json](initial_slice.json).
Detailed receipts stay under these ignored paths:

- `work/dropin_build_final/build_receipt.json`, `tests.xml`, and `Testing/Temporary/LastTest.log`.
- `work/dropin_mesh_final/dropin_geometry_replay_report.json` and `geometry_diagnostic.json`.
- `work/dropin_validation/historical.json`, `live_capture.json`, `command01_extent.json`.
- `work/dropin_validation/portable_x86_final/portable_tests.json` and `work/renderer_contract_tests/verification.json`.

Initial VC5 sandbox launches failed with Windows error 623 before compilation.
Those launch failures remain distinct from compiled experiments. The unchanged
compiler/flags and source-built fixtures succeeded through the documented
fallback. A consumer receipt-path spelling mismatch was corrected in the modern
validator; the final end-to-end public runner passes on fresh outputs.

## Next concrete boundaries

Original post-entry interception still requires execution authorization and
demonstrated writable-path/profile/registry isolation, windowed startup and
restoration. The production proxy is not installed alongside any user game.
Its process-lifetime explicit adapter remains separate from DllMain publication.

The next useful renderer slice is texture/CLUT state plus reachable rectangle
commands feeding the existing bounded indexed reference. That can progress
offline while original-host isolation is investigated. Visibility, writer
ordering and exclusive final presentation remain open. SDL's successful x86
diagnostic establishes a backend candidate; it does not settle those boundaries.
