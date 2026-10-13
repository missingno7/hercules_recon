# Presentation ownership and bounded probe status

## Evidence boundary

The available host observation is the separate pre-entry publication receipt:
the app-local diagnostic DLL was selected and published a 250-DWORD table
clone before the pinned EXE's first instruction on the tested Windows setup.
That fixture returned `E_NOTIMPL` from its diagnostic `DirectDrawCreate` and
terminated at the entry breakpoint. It did not initialize the game, call an
engine, or display a frame. See
[`preentry_publication.md`](preentry_publication.md).

The source-built forwarding fixture in
[`fixtures/presentation_probe`](fixtures/presentation_probe/README.md) has
separate synthetic ABI/recursion-guard evidence and one historical
source-built real-system DirectDraw API result. Neither is original-Hercules
forwarding evidence. The first raw API receipt is preserved unchanged at
[`evidence/presentation_forwarding_first_raw.json`](evidence/presentation_forwarding_first_raw.json)
with status `UNEXPECTED_FORWARD_MODULE_PATH`. A later metadata-only process
matched its logical module path and export RVA to the pinned physical system
DLL, but that comparison is cross-process. The fresh public-source API replay succeeded with HRESULT zero, a returned
object and Release return zero. It is a separate item and records module path, canonical file path, mapped
PE identity, HRESULT, object, and Release in the same source-built process.
It did not launch the original EXE or engine. The logical System32 path and
canonical SysWOW64 file path, mapped PE identity and pre/post physical file
hash were checked in this same source-built process.

The candidate's original-Hercules forwarding status is **UNRESOLVED**. No
`HOST_INTEGRATION_OBSERVED` claim applies to this candidate.

## Fixed-resolution presenter versus full-native-resolution ownership

The host's windowed presenter consumes an 8-bit indexed 320x240 DIB; its
fullscreen DirectDraw path also selects the original 320x240 mode and palette.
Rendering privately and copying into the current locked view is therefore a
fixed-resolution compatibility experiment. The view pointer and pitch must
be reacquired on each lock; unlock clears the lock state but leaves those
cells stale. The original palette remap, dirty latch, clear and final present
remain host-owned. Other host writers can still modify the indexed view and
presentation state while a GPU copy is being prepared.

Using a GPU swapchain with the original HWND does not transfer display
ownership. The host still creates and releases DirectDraw objects and
surfaces, changes cooperative level and display mode, assigns palette state,
handles resize/window messages, and writes through DIB/GDI, clear/remap, and
present paths. Full native resolution needs a coordinated display owner that
controls those transitions and the final frame path. A separate owner also
needs an explicit shutdown sequence that releases its resources and restores
the host window, palette, cooperative level, display mode, and desktop state.
No such GPU implementation or shutdown sequence has been observed or built.

The writer inventory still has independent composition routes. AC60 submits
the world and invokes direct upload/VRAM-move services; TITLE has a movie
callback at table slot 148. Table slot 149 points to host helper `0x6440`,
which can draw through the attached DirectDraw back buffer (`TextOutA` at
`0x64b7`) or through `GetDC(HWND)`/`TextOutA`/`ReleaseDC` (`0x650c`). It does
not touch the reviewed lock view or dirty byte, and its caller and visibility
timing remain unresolved. Framework paint/GDI imports are further unowned UI
routes. Palette/LUT writes and texture-memory versus CPU-visible framebuffer
updates also have distinct ownership. A passive slot-81/89 trace cannot show
that these routes are quiescent or serialized.

Therefore the original indexed presenter is the narrower route for a
faithful 320x240 experiment. Full native resolution requires complete
world/UI/movie ordering, palette and index policy, control of all mode and
window writes, resize handling, serialized framebuffer access, and tested
shutdown restoration. The existence of an original HWND does not prove any
of those services are owned by an independent renderer.

## Narrow observation boundary

The candidate clones exactly 250 DWORDs and substitutes only slots 81 and 89
with stable wrappers. The wrappers record fixed-size raw arguments, saved
callback addresses, and before/after boundaries, then call the original
callback synchronously. They do not walk packets, read linked order-table
memory, write files, allocate during callbacks, or invoke graphics APIs. The
bounded ring holds 1,024 events; overflow increments a dropped counter.
Capture must suspend all process threads only after a completed wrapper
return before copying the ring, and must reject an overflowed trace. A slot-89
call by itself is a submission callback, not proof that a displayed frame
completed.

The pinned retail EXE has SHA-256
`587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`. The
reviewed no-CD variant is a distinct file with SHA-256
`b3ab3ee9e5c973bb245d8eed795e8e7065d7dfc371e412e602b2059af894a551`; the
bounded renderer/loader/table spans do not overlap its single observed byte
difference at file offset `0x94b0` (RVA `0xa0b0`). Any host-side probe must pin
the exact intended executable and its file hash before startup rather than
infer identity from table-layout checks.

The generic engine loader releases an earlier module, calls `LoadLibraryA`,
and resolves `PC_DLLEngineMain`. Although the host contains names for ENG3,
ENG1, and TITLE, it extracts basenames before loading; the actual path,
order, and loop count remain unobserved. Any later observation must record
each loaded module's resolved path and hash and stop if it differs from a
reviewed original. The renderer fixture does not run this loader.

The host's initial route requests fullscreen. A windowed observation would
need to verify both requested and current mode bytes are zero and confirm the
restored caption and menu before accepting a slot-89 stop; the 330x293 popup
without a menu remains on the fullscreen path. The candidate stop point for
one trace sample is after a completed slot-89 wrapper return with the normal
window visible, before the next host instruction. That still would not prove
the image reached the screen. Do not send input or enter gameplay to force a
callback.

## Isolation and cleanup prerequisites

Copying the EXE into `work` alone is not a write-isolation boundary. The host
may consult registry-configured install paths and may write saves or config
outside its current directory. Before a root-approved host run, identify
those registry reads and filesystem writes, use a disposable profile and
registry view, preserve user configuration, make original assets read-only,
and confine writable paths to a verified disposable root. Pin the exact
retail/no-CD image and reviewed original engine DLLs first. The original
pre-entry observation does not authorize startup beyond the entry breakpoint.

Any live-window probe also needs an agreed emergency stop/cleanup path that
preserves configuration and restores display mode, cooperative level, window
style/menu/cursor, and desktop state. Those operations have not been tested
for this candidate. Root review is required before starting any original
Hercules host or engine process. This document proposes no gameplay run.
