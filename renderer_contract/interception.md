# Interception feasibility

## Recommended boundary

Use a persistent **250-word callback-table clone**, published through host
pointer cell RVA 0x365A0 before the first cold PC_DLLEngineMain call. Preserve
every unselected word, including shared data pointers. Each original engine
copies the prefix once into its own storage; publishing later does not update
an engine already initialized. This avoids runtime instruction detours and
permits unchanged original EXE and engine files.

Begin the future renderer with a passive wrapper for slot 89 (+0x164): record
the raw ordering-table stream synchronously and forward to original AC60.
For module-independent safe traversal, also observe slot 81 (+0x144) while
forwarding its initializer unchanged: record each table address and positive
count. ENG1/TITLE use 0x500 slots; ENG3 uses 0x800. The submit callback itself
contains no count. These observations avoid inferring an extent from a linked
pointer or assuming one common frame aggregate.
Capture must respect borrowed packet lifetime and submit order. The safe
package walker adds bounds absent from the original; a rejected capture must
not alter the game's native callback behavior.

A GPU replacement for AC60 is a larger contract. With BA01FC zero, the original
optionally initializes surface state, locks, draws, calls 70C0(0xFF), unlocks,
then presents. BD80 runs last on both gate paths. E1 page changes, command 01
VRAM moves and F3 clipping remain active on the nonzero gate path. Required
state writes and notification must survive even when rasterization is bypassed.
The [ownership audit](ownership.md) finds the packet workspace and page state
direct-reference-local to the reviewed closure, but does not exclude aliases.
Keep observed staged writes reproducible. Surface/pitch, lock state, dirty
state and the draw gate have other host consumers, including movie display.
They must remain coherent with preserved host services.

Completion can be resolved at verified host image base plus RVA 0xBD80.
Forwarding saved slot 80 is a different wrapper: AC00 calls BD80, then clears
EAX, and adds a callback return frame. Preserve the direct gate contract rather
than claim those entry points are interchangeable. Live invocation remains
unobserved.

The [primary notification audit](notification_target.md) proves slot 92 is a
one-argument cdecl raw-address exchange returning the previous address. The
five direct engine registrations all pass NULL, and BDF0 enables the gate
while clearing primary and secondary targets. No non-null target body is
identified. Computed installers remain unresolved, so retain BD80 and its
synchronous position rather than remove it based on these direct paths.

Preserving unselected table words also preserves unusual trampoline behavior.
The [context-writer audit](upload_domain_bounds.md) finds host slot 19 reading
the original DLL caller's second argument beyond the trampoline's two copied
arguments. Its stack relation and shared context pointer are part of the
observed contract. Leave that service and its existing trampoline intact.

Texture coherence requires +0x204/A710 uploads, TITLE +0x208/A640 convenience
uploads and +0x20C/A8F0 VRAM moves. A640 calls A710 directly; AC60 calls A8F0
and 70C0 directly. Wrapping those table slots alone does not intercept every
call. Preserve original callbacks and their return/notification order while
observing updates, or implement the corresponding direct paths inside the
replacement submission contract. Do not snapshot a guessed contiguous VRAM
bank: page-address arithmetic does not prove exclusive memory ownership.
Nested BD80 notifications can introduce later updates before an outer upload
wrapper returns. Mirroring an outer upload only after return can reverse that
order. Aliasing source/destination memory also prevents assuming that a copied
input rectangle predicts the final native map. A checked read-at-submit view
of required native pages/LUTs is a candidate way to retain original ownership;
it still needs validated read bounds and state-command ordering. No blind bank
snapshot or asynchronous borrowed-pointer use is part of this contract.

## Loading versus interception

The EXE statically imports DDRAW.dll!DirectDrawCreate. Its call at RVA 0x7426
is in graphics initialization before the observed first engine-table copy.
An app-local forwarding ddraw.dll is therefore a candidate bootstrap for
publishing the clone. It should preserve the real DirectDraw object and load
the system DLL by an explicit system path at call time. Keep DllMain minimal.
Actual local-DLL resolution and timing remain unobserved. Windows considers
loaded modules and KnownDLLs before the application's directory, so deployment
must verify the loaded path rather than assume selection.
[Microsoft DLL search order](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-search-order),
[DLL best practices](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-best-practices).

The proxy loads the interception mechanism; DirectDraw itself is not the
polygon-packet interface. No proxy or injection code is implemented here.

| Approach | Original file changes | Scope and reliability | Cross-module and presentation cost |
|---|---|---|---|
| Direct AC60 detour | None on disk; runtime instructions changed | Single shared host target, but requires safe instruction interception and complete outer effects | Covers three module submit routes; misses movie and other presentation paths |
| Selected callback clone | None; runtime data pointer changed | Smallest observed renderer seam; must precede cold copies and preserve borrowed state | Shared across three modules; TITLE extra upload/movie routes require explicit handling |
| Forwarding ddraw.dll bootstrap + clone | Add local proxy; originals intact | Statically early enough; loader selection needs bounded deployment proof | Retains original window/COM services; no replacement rendering supplied by proxy alone |
| Engine DLL proxies + clone | Originals must be reachable under separate names/paths | Three forwarding wrappers, export/return forwarding and module search complexity | Handles per-module copy directly; larger installation/maintenance burden |

There is no mutable per-polygon dispatch table inside AC60. Its four polygon
and sprite rasterizer calls are direct. Hooking only DirectDraw presentation
can capture the already rasterized image; it does not expose primitive data.

## Presentation choices

For the first faithful GPU experiment, retain the original host surface,
palette, HWND, movie path and presentation services. An offscreen indexed
render result copied into the original locked 320x240 surface could preserve
the native final color remap and presenter. This is an architectural proposal,
not a measured implementation; synchronization and indexed blend equivalence
still require validation.

A native GPU swapchain using the original HWND would need coordinated palette,
movie, resize/window-message and fullscreen transitions. Original DirectDraw
and GDI writers cannot be allowed to race an independent presenter. These are
separate full-presentation blockers, not blockers for passive packet capture.

## Enhanced geometry

Final packets contain projected XY and UV/state fields, without per-vertex Z,
reciprocal depth or normals consumed by these routes. They support faithful
ordered GPU rasterization and optional image presentation improvements. True
perspective-correct texturing/depth needs earlier geometry capture and reliable
association with emitted/clipped primitives. Existing host transform callbacks
expose camera coordinates, but that does not establish a complete capture and
association protocol across every module. See geometry.md.

Even faithful screen-space rendering must reproduce the native scan arithmetic.
The [packed-UV witness](packed_uv_counterexample.md) rejects independent Q16
pixel advancement for one wire-representable quad. Ordinary triangle splitting
and continuous interpolation are also falsified by existing finite fixtures.
The backend must resolve these sampled-address semantics before claiming
faithful polygon rendering.

## Preserve live traversal and aliased tables

AC60 follows the current node's link at ACA2 after dispatch, including an
A8F0→BD80 notification. A replacement must preserve callback positions and
reload native links and shared state afterward. Predecoding a whole frame
before callbacks can discard later writes. The offline bounded walker is a
capture aid, not an execution substitute. Notification ownership is a separate
contract; retaining the original service is the conservative boundary.

Native LUT construction proves actual table aliasing. Shade/CLUT/blend bytes
cannot be snapshotted as independent owned textures. Original window, palette,
clear, movies and final surface remap remain required compatibility behavior
at the initial clone boundary. No bootstrap/window coexistence has been run.
