# Hercules PC presentation boundary

## Finding

The renderer seam is the selected callback-table clone, rather than DirectDraw
presentation itself. An app-local forwarding DDRAW.dll is a candidate loader,
but its actual selection and publication timing in Hercules remain unobserved.
The independent guarded event path prevents inferring a process-wide first-copy
order from the DirectDraw call alone; see [bootstrap order](bootstrap_order.md).

The PE import descriptor lists `DDRAW.dll` and `DirectDrawCreate`. This is a load-time import, so Windows loads its dependency during process initialization before the EXE begins normal execution; Microsoft describes this in [Load-Time Dynamic Linking](https://learn.microsoft.com/en-us/windows/win32/dlls/load-time-dynamic-linking). Descriptor order does not establish the runtime DLL path. The loaded-module list and Known DLLs are checked before the executable directory in the standard unpackaged search order. Whether `ddraw.dll` is Known on a particular Windows version and whether an app-local wrapper is selected must be observed on the target deployment; this report does not claim that a local proxy definitely loads. See Microsoft's [DLL search-order documentation](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-search-order).

If app-local resolution is unavailable, a proxy for the dynamically loaded engine DLL could intercept `PC_DLLEngineMain` and pass a cloned table to the original entrypoint. That is a later, per-engine boundary and would need to cover each engine module the host loads. The DirectDraw hook is the narrower shared point because the host source-pointer cell is global and read at each engine entry.

## What the original presentation path owns

The HWND is stored at VA `0xBA0200`. Graphics setup obtains its HDC, allocates and initializes a 320Ä‚â€”240 top-down 8-bit DIB (`BITMAPINFOHEADER`: size `0x28`, width `320`, height `-240`, one plane, eight bits, `BI_RGB`), creates a GDI palette, then calls `DirectDrawCreate`. The EXE also imports `DSOUND.dll!DirectSoundCreate`, `WINMM.dll`, `USER32.dll`, and `GDI32.dll`; the relevant window and GDI calls include `CreateWindowExA`, `GetMessageA`, `CreateDIBSection`, `StretchDIBits`, and palette operations.

The EXE's working DirectDraw globals are `IDirectDraw*` at RVA `0x56e3e4`, primary surface at `0x56e3e8`, attached back-buffer at `0x56e3ec`, and DirectDraw palette at `0x56e3f0`. The windowed side owns `BITMAPINFO*` at `0x56e3f4`, `LOGPALETTE*` at `0x56e3f8`, the created `HPALETTE` at `0x56e3fc`, HDC at `0x56e400`, HBITMAP at `0x56e404`, and DIB pixel pointer at `0x56e3d8`. The active lock view is pointer/pitch at VA `0xBA0210`/`0xBA0214`; VA `0xBA020C` selects the DirectDraw/fullscreen path. The lock-state byte is at `0xBA01F0`.

On the fullscreen transition, the host changes the original window's styles, menu, size, and cursor state, sets the DirectDraw cooperative level using the original HWND (native mode values `0x55` on entry and `0x0c` on exit), and changes to 320Ä‚â€”240Ä‚â€”8. Surface setup releases prior COM objects, builds a `DDSURFACEDESC` with `dwSize=0x6c`, `dwFlags=0x21`, and `ddsCaps=0xa18`, asks for a primary flip chain with two back buffers and falls back to one, obtains the attached back buffer, creates a 256-entry `DDPCAPS_8BIT` palette, and assigns it to the primary surface. The palette loop writes zero RGB and `peFlags=4` for each entry. The descriptor does not set a pixel-format flag or an explicit `DDPIXELFORMAT`, so the evidence supports the 8-bit display mode and palette but no separate explicit surface bit-mask. On exit it copies back to the DIB, restores display/cooperative mode and the HWND placement/styles/menu/cursor, then invalidates and presents the window.

Lock RVA `0x6ff0` checks whether the primary surface was lost (`IDirectDrawSurface::IsLost`, vtable byte offset `0x60`) and restores it (`Restore`, `0x6c`) on `DDERR_SURFACELOST`. It then locks the attached back buffer (`Lock`, `0x64`, wait flag set) and publishes the returned pixels and pitch. In the non-DirectDraw mode it instead exposes the DIB pointer with pitch 320. Unlock RVA `0x7140` calls `Unlock` at `0x80` for the DirectDraw path. Present RVA `0x6620` flips the primary surface at `0x2c` with wait flag in fullscreen; windowed it calls `StretchDIBits` and `GdiFlush` using the original HDC and DIB. If the image/DC is unavailable it clears the window with `FillRect`.

Vtable byte offsets in the prior paragraph are confirmed by the native indirect-call operands and the pinned VC5 `ddraw.h` interface order: `IDirectDraw::CreatePalette` `0x14`, `CreateSurface` `0x18`, `SetCooperativeLevel` `0x50`, `SetDisplayMode` `0x54`, `WaitForVerticalBlank` `0x58`; `IDirectDrawSurface::Release` `0x08`, `Flip` `0x2c`, `GetAttachedSurface` `0x30`, `IsLost` `0x60`, `Lock` `0x64`, `Restore` `0x6c`, `SetPalette` `0x7c`, `Unlock` `0x80`; `IDirectDrawPalette::SetEntries` `0x18`.

## Paths that remain when packet consumption is bypassed

The table maps packet consumer RVA `0xac60` at slot 89 / offset `0x164`, texture rectangle upload `0xa710` at slot 129 / offset `0x204`, and color translator `0x70c0` at slot 143 / offset `0x23c`. The host has no direct-call xref to `0xac60`; the engine reaches it through the copied interface. `0xa710` is also called directly by host helper `0xa640` at `0xa675`. `0x70c0` is both a table callback and an internal direct callee of the packet consumer at `0xb2d0`, so replacing only its table cell does not replace that internal call.

At the tail of the packet consumer, the host calls `0x70c0(0xff)` to transform the locked 320Ä‚â€”240 8-bit buffer through a palette/color table, then unlocks at `0xb2d8` and presents at `0xb2dd`. This demonstrates why a packet-consumer callback is not itself the complete display boundary.

The independent movie display callback at RVA `0x5450` has its own calls to lock (`0x55e8`), unlock (`0x5672`), and present (`0x5677`). The movie protocol evidence identifies this host callback as the engine's movie-display service and the movie pixel data as borrowed host storage. Bypassing `0xac60` therefore does not remove the movie path's use of the original screen buffer and, in fullscreen, the DirectDraw back buffer and flip.

The host also retains its palette and window-message paths. Palette refresh RVA `0x69a0` unlocks/presents around palette updates, uses `IDirectDraw::WaitForVerticalBlank` (`0x58`) and `IDirectDrawPalette::SetEntries` (`0x18`) in DirectDraw mode, and uses GDI palette calls in windowed mode. The handler at `0x6700` recognizes message `0x311`, selects and realizes the GDI palette, then calls the presenter. Thus the windowed overlay/palette path is GDI-backed; the fullscreen display, palette, and movie paths still depend on the original DirectDraw objects.

## Bootstrap constraints and open checks

A future forwarding wrapper must preserve the exact imported export name,
arguments, HRESULT and original COM object. Defer real system-DLL loading, COM,
window and GPU initialization outside DllMain. Minimal early data publication
is a candidate for reaching the first possible cold copy; the
[source-built fixture](fixtures/early_loader_probe/README.md) establishes this
only for its synthetic EXE. Publishing on HRESULT success alone is unsupported:
the host uses that result to select logging and continues on both branches.
See [loader constraints](loader.md) and [Microsoft DLL best practices](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-best-practices).

The clone must preserve unselected entries and data-pointer entries as well as function entries. It should be allocated for process lifetime, because each engine's `PC_DLLEngineMain` copies the table prefix into module-local storage. Updating the source pointer later does not broadcast into an already copied engine table. Actual module-path resolution, actual loaded module path, live window mode transitions, and callback timing remain unobserved; they are the bounded integration checks before treating this bootstrap as viable on a target installation.

## Reproduction and provenance

Run `python -B work/renderer_presentation/audit.py` from the repository root. The script reads the pinned oracle through `work/renderer_baseline/baseline.py`, checks its expected SHA-256, emits deterministic JSON, and writes nothing. Its output is saved in [evidence.json](evidence/presentation.json). The compact source spans, source-table slots, direct-call xrefs, import thunk, and reused evidence hashes are recorded there.

Pinned COM declaration used for cross-checking: `C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953\include\ddraw.h`, SHA-256 `fcaadaaf15d5ee85db3dd6a40f1eee2f0dd7aabcecc494579022c55d063ecf34`. Oracle SHA-256 is `587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`. No game code, assets, recovery state, or runtime image was changed or executed.

The earlier direct-path timing prediction is superseded by the narrower
[bootstrap audit](bootstrap_order.md). Actual app-local selection, independent
callback scheduling and early publication require separate target observations.

## Integrated reproduction

The private investigation paths above identify provenance. In this package, run tests/validate_originals.py with the original image paths to recheck recorded bytes and bindings, and tests/run_tests.py for the compiled synthetic suite. The byte validator does not independently redo the semantic analysis. The compact receipt is evidence/presentation.json.
