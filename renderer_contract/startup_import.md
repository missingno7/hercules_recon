# Static DirectDraw attach and host cold-copy ordering

## Result

For ordinary Windows process startup, the new PE audit supports a conditional **yes**: if the `DDRAW.dll` module selected for HERCULES's non-delay static import is the candidate app-local module, and its process-attach entrypoint returns success, its `DLL_PROCESS_ATTACH` initialization precedes host EXE entry code. The host's `0x79a0` scheduling path is executable code, so it cannot run before this attach on that startup path. This gives an earlier publication point than waiting for `DirectDrawCreate` to be called at `0x7426`.

This is a Windows loader ordering result composed from the documented static-import startup and DLL process-attach behavior; it is not evidence that a candidate module is selected for HERCULES on a deployment. The existing synthetic fixture demonstrates early-attach publication versus deferred API publication for its own mock process only. No host process was launched.

## Pinned-image facts

`HERCULES.EXE` SHA-256 is `587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`; PE32/i386, image base `0x400000`, entry RVA `0x11ea0` (VA `0x411ea0`). The pinned image has a normal import directory, including static `DDRAW.dll!DirectDrawCreate` at IAT RVA `0x7a5648`. Its thunk is at `0x10a20`, 6 bytes with SHA-256 `ed64febc50f25778a8a17187ad15903925a528a4682a055bda4b01f901bd7152`; the bounded full `.text` sweep finds its callsite at `0x7426`.

The image has no TLS directory or `.tls` section, so it declares no EXE-owned TLS callback array. It has no CLR runtime header, delay-import directory, or RT_MANIFEST resource; no image exports; and no statically imported engine-named module. The dynamic engine boundary is instead `LoadLibraryA` candidates at `0x5c75` and `0x19403`, `GetProcAddress` at `0x5c95`, `0x13845`, `0x202f0`, and `FreeLibrary` at `0x5c3a`, `0x2cd3a`. The string at RVA `0x36868` is `PC_DLLEngineMain`; the existing loader audit associates lookup with the separate dynamic engine loader.

The import descriptor file order begins `DDRAW.dll`, `DSOUND.dll`, `WINMM.dll`, `KERNEL32.dll`, `USER32.dll`, `GDI32.dll`, `WINSPOOL.DRV`, `ADVAPI32.dll`, `SHELL32.dll`, `COMCTL32.dll`. That is on-disk descriptor order, not observed module initialization order. The host table dispatch reads its published pointer at `0x5ef3` and calls the loaded engine export at `0x5f03`; its dynamic engine module is not a static import.

## Deployment boundary

The static import guarantees a loader dependency, not which file satisfies the name. The Microsoft search-order documentation places redirection, API sets, the loaded-module list, and Known DLL handling ahead of the application directory for the documented unpackaged search order; therefore app-local selection is conditional on the target's activation context, KnownDLL state, loaded modules, sidecar/redirection policy, and deployment layout. The pinned install has no app-local `ddraw.dll`, `.local` sidecar, or embedded manifest. The separate current-machine receipt observed no `ddraw.dll` KnownDLL value and the source-built surrogate selected its sibling stub, but neither result proves HERCULES resolution elsewhere.

The smallest viable timing claim is: **when** normal startup binds the static DDRAW import to the intended module, a minimal attach-time publication can precede all host EXE-controlled copies, independent of `0x79a0` scheduling. The module should limit attach work to simple validated static-data publication. It must not load the real DirectDraw implementation, initialize COM/window/GPU services, or perform other heavy forwarding from `DllMain`; those operations remain deferred. The previous surrogate does not establish whether a product proxy can preserve the original DirectDraw object, exports, HWND, or host surface services.

Unresolved externally: target import resolution and successful attach, the module's complete required export/dependency behavior, nonstandard injected/custom loaders, and live confirmation that HERCULES's first engine-table copy sees the intended clone. The source-built surrogate's `early_loader_surrogate.json` is labeled surrogate-only and cannot close these points.

## Reproducible audit

Private public-ready audit: `audit_import_preentry.py` reads only the pinned PE, checks `scripts/pe.py` SHA-256 `8da71eb87b5a88dd1d80a3fe180e2b94557e2c3017fb4e5c37c4a8c630af62ca`, and writes under repository `work/`, outside the original image directory. It records all 16 PE data directories, imports/exports, entry point, TLS callback array (if present), relevant import call candidates, engine string, and complete skipdata `.text` coverage. It copies no code bytes into its receipt. Run:

```powershell
& $pythonExe -B renderer_contract\tests\audit_import_preentry.py `
  --original-root D:\Prog\hercules_recon\work\renderer_interface_repo `
  --exe D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE `
  --output work\replay\startup_import.json
```

This run produced `startup_import.json` with full raw `.text` coverage (183,808 bytes, 65,375 valid instructions and four skipdata pseudoops), zero EXE TLS callbacks, zero exports, a static DDRAW import, and the dynamic engine resolution sites above. It relies on the same pinned existing helper/dependency set as `audit_loader.py`; it adds no external package.

## Windows documentation

- [Microsoft: API set loader operation](https://learn.microsoft.com/en-us/windows/win32/apiindex/api-set-loader-operation) states that static imports resolve before code runs. This audit applies that startup guarantee to HERCULES's ordinary static DDRAW dependency.
- [Microsoft: DllMain entry point](https://learn.microsoft.com/en-us/windows/win32/dlls/dllmain) documents process-attach notification on process startup, the static-load indication, and restrictions on work performed under the loader lock.
- [Microsoft: PE format](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format) defines the import, TLS, export, delay-import, and CLR directories and the TLS callback array.
- [Microsoft: Dynamic-link library search order](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-search-order) documents redirection, loaded modules, Known DLLs, and the application-directory position in search order.

## Evidence identity

The private receipt is `startup_import.json`. Existing correlated evidence, retained without modification:

- `renderer_contract/evidence/loader.json` SHA-256 `a1bb78073f539abd553439fbc28e5733027354e2bf09fad429f19633a36f7649`.
- `renderer_contract/evidence/bootstrap_order.json` SHA-256 `ff1237e664d6779b400ec25dd9b7585f4db044bcf7a4c66888ac2fee08336832`.
- `renderer_contract/evidence/early_loader_surrogate.json` SHA-256 `0f2f8e1921d2d1cbce56e9f9f121b9cadb25222710b9e9914bdc29aa999d2a3b`.
- `renderer_contract/evidence/loader_environment.json` SHA-256 `df3b78f78af7f76ec0c29edae014114a60d7942a4ae4ba4a1ba53775405a814cc`.
- `renderer_contract/tests/audit_loader.py` SHA-256 `ea506b6ec770109fe5cefd2347a4c24ccb52eedd359470ec81b26a1b5f8679e3`.

The strict `.text` decoder's previous stop at RVA `0x1124a` is prefix-only; this audit uses complete skipdata coverage. Linear-sweep call candidates are not control-flow reachability proof.

The public receipt is [startup_import.json](evidence/startup_import.json). [Microsoft Load-Time Dynamic Linking](https://learn.microsoft.com/en-us/windows/win32/dlls/load-time-dynamic-linking) documents dependency mapping and load-time entry-point invocation for ordinary DLL imports.
