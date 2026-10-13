# Phase 4: loader and presentation bootstrap feasibility

## Finding

The pinned host has a narrow, statically ordered pre-engine boundary suitable for a data-only renderer-table redirect. `HERCULES.EXE` calls its imported `DDRAW.dll!DirectDrawCreate` at RVA `0x7426`, after graphics setup reaches `0x71a0`, and before the first source-table pointer read at `0x5ef3` and first indirect `PC_DLLEngineMain` call at `0x5f03`. A shim can therefore forward `DirectDrawCreate` unchanged, then clone the 250-DWORD source table and publish the clone through host pointer cell RVA `0x365a0` before returning. The engine modules copy that table on their first entry, so the update must precede `0x5f03`; keep the clone alive for process lifetime.

This is a static timing result, not observed host integration. A source-built x86 surrogate confirmed that a normal static `DDRAW.dll!DirectDrawCreate` import resolved to an app-local DLL on this current Windows machine. The original game was not launched, and actual deployment resolution, system-forwarding success, and host window/surface behavior remain unverified.

## Host imports and lifecycle

The oracle is PE32/i386 (`HERCULES.EXE`, SHA-256 `587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`, 275,968 bytes), preferred image base `0x400000`, entry RVA `0x11ea0`. It has no base-relocation directory, delay-import directory, RT_MANIFEST resource, or nonzero loader flags. The pinned install directory has no `HERCULES.EXE.manifest`, `.local` sidecar, or app-local `ddraw.dll`.

The PE import descriptor file order is `DDRAW.dll`, `DSOUND.dll`, `WINMM.dll`, `KERNEL32.dll`, `USER32.dll`, `GDI32.dll`, `WINSPOOL.DRV`, `ADVAPI32.dll`, `SHELL32.dll`, `COMCTL32.dll`. This is file-layout evidence, not observed runtime dependency load order. The host's `DirectDrawCreate` IAT is RVA `0x7a5648`; one call goes through host thunk RVA `0x10a20` at RVA `0x7426`. It passes GUID `NULL`, output address `0x96e3e4`, and outer `NULL`; the host stores/checks HRESULT and logs failure while continuing.

The separate engine-module loader calls `LoadLibraryA` at `0x5c75`, resolves `PC_DLLEngineMain` with `GetProcAddress` at `0x5c95`, and can call `FreeLibrary` at `0x5c3a`. It derives a basename from a mutable path. Which module path it selects is not established here. The init sequence is host init `0x5e10` → graphics setup `0x72a0` → DirectDraw setup `0x71a0` at callsite `0x741a` → DirectDraw import call `0x7426`; first source-pointer read is `0x5ef3` and first engine dispatch call is `0x5f03`.

## Redirect table scope and retained presentation paths

The static source table is 250 DWORDs (1,000 bytes) at RVA `0x361b8`, VA `0x4361b8`, in `.data`, SHA-256 `52654fb050caa552b03c692bd3a1da0e700675bb50605394c8afb92595a039f8`. The pointer cell at RVA `0x365a0` initially contains `0x4361b8`. A full raw `.text` linear sweep with explicit skipdata coverage found zero candidate fixed-address writes to all 250 words, zero recognized absolute indexed writes into the table, and one pointer-cell reference: a read at `0x5ef3`. This candidate inventory does not prove code/data instruction alignment or exclude external mutation and unresolved aliases. The earlier strict sweep stopped at RVA 0x1124A; the corrected sweep covers 183,808 bytes, including four explicitly skipped bytes.

The narrowest candidate is an app-local `DDRAW.dll` export shim for `DirectDrawCreate` with the x86 `HRESULT WINAPI DirectDrawCreate(GUID FAR *, LPDIRECTDRAW FAR *, IUnknown FAR *)` ABI. Load the real implementation by explicit system path, forward the original arguments and HRESULT/object unchanged, and—only after a valid DirectDraw result—copy the static table, replace only selected callbacks, and write the clone pointer at `image_base + 0x365a0` before returning. Confirm the actual image base against `0x400000`, since the executable has no relocation directory. The chosen slots and callback ABI belong to the interface contract; this report does not authorize replacing the DirectDraw device or window.

Preserve the original HWND, DirectDraw object and COM vtables, cooperative/display-mode transitions, surfaces/backbuffers, palette writes and updates, lost-surface recovery, lock/unlock/clear/present, and movie path. Existing presentation evidence records observed COM byte offsets: `IDirectDraw::{CreatePalette 0x14, CreateSurface 0x18, SetCooperativeLevel 0x50, SetDisplayMode 0x54, WaitForVerticalBlank 0x58}`, `IDirectDrawPalette::SetEntries 0x18`, and `IDirectDrawSurface::{Flip 0x2c, GetAttachedSurface 0x30, IsLost 0x60, Lock 0x64, Restore 0x6c, SetPalette 0x7c, Unlock 0x80, Release 0x08}`. Movies retain the slot-148 callback and their surface path; the original resource/palette callback at slot 145 and host palette writers also remain in place. These existing services are needed even if a future experiment bypasses the packet consumer.

## Current Windows loader probe

On Windows `10.0.26200.0` (64-bit), read-only queries found no registry value named `ddraw.dll` in either Registry64 or Registry32 view of `KnownDLLs`; the queried `KnownDLLs32` keys were absent. The pinned install directory has no ddraw sidecar. Both system ddraw binaries exist, with x86 `SysWOW64\ddraw.dll` SHA-256 `37113406967162585c9a67c252005757459d8efe87684bbc0ce184907d0566b0` and x64 `System32\ddraw.dll` SHA-256 `bf254d03be6e7756799de9f9602df062c49d7aaafade781c9769b2360c752f64`.

A private VC5-built x86 probe with a static `DDRAW.dll!DirectDrawCreate` import loaded its sibling source-built stub (`MODULE=<surrogate>\ddraw.dll`). A control copy with no sibling stub loaded a module reported as `C:\WINDOWS\SYSTEM32\DDRAW.dll` by the x86 process; this string is preserved exactly as observed and does not identify the physical WoW64 mapping. The import address was inspected but `DirectDrawCreate` was never invoked. These runs establish ordinary app-local selection for the surrogate on this machine only. They do not prove the game resolves the same way or that a shim can safely forward under the intended deployment.

Microsoft's documented unpackaged-app search order checks Known DLLs before the application directory, so a target installation where `ddraw.dll` is registered as a Known DLL may bypass an app-local shim. See [Dynamic-link library search order](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-search-order). Search policy and side-by-side/deployment context vary; do not infer a universal app-local result from the surrogate.

## Prediction and falsifier

**Prediction:** on the pinned host, if a compatible app-local shim is selected and `DirectDrawCreate` reaches its export, the shim can publish the clone before any engine module copies the table; no later host `.text` write will restore the original source pointer or fixed table entries.

**Falsifier:** reject this boundary if bounded host integration shows the first engine table copy precedes `0x7426`, observes a different pointer at the first copy, or shows a later host write clobbering the published pointer/table. Reject the app-local deployment candidate if the target loader resolves `DDRAW.dll` elsewhere or the shim cannot forward to the genuine implementation while preserving the original result and object. Until those checks exist, retain the original HWND and DirectDraw/palette/movie services.

## Reproduction artifacts

From the checkout root, use the public audit and source-only loader fixture:

```powershell
python renderer_contract/tests/audit_loader.py --original-root . --exe D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE --output work/loader_audit.json
python renderer_contract/fixtures/loader_probe/build_and_run.py --original-root . --output-dir work/loader_surrogate
```

The build output must be new or empty and inside checkout/work. The builder
verifies the installed VC5 manifest and DX6 import-library hash. Compiler
products, logs and the full receipt stay there. It never starts Hercules or
calls DirectDrawCreate. The tiny stub is an import-selection fixture and must
not be installed in a game directory.

Compact receipts are [loader.json](evidence/loader.json),
[loader_surrogate.json](evidence/loader_surrogate.json) and
[loader_environment.json](evidence/loader_environment.json). Source, compiler
and original-byte identities remain separate. No production proxy or renderer
is implemented here.
