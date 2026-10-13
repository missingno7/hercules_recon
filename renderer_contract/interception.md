# Interception feasibility

## Native-resolution extension

The table clone remains the smallest shared host-service boundary. It is
insufficient by itself for exact high-resolution ENG1 geometry association:
point/XY callbacks do not expose selected face indices. The bounded producer
fixture identifies pre-face observers at ENG1 `2E0F1` (GT4) and `2E27B`
(FT4), joined only when packet emission is confirmed. Runtime observation
of those instructions would be an additional future mechanism; no detour is
implemented here and on-disk originals remain unchanged.

ENG3 and TITLE independently use batch slots 102/107/117 before their
fourth-point 101/106/114 calls. Their geometry cannot be captured through slot
106 alone. Full native-resolution presentation also requires coordinated
surface/GDI/palette/movie ownership. See [readiness](native_resolution.md),
[association](eng1_geometry_association.md),
[cross-module geometry](cross_module_geometry.md) and
[presentation takeover](presentation_takeover.md).

Texture coherence likewise cannot rely solely on replacing upload callbacks:
A640 calls A710 directly, and AC60 reaches A8F0 directly. Startup 71A0 and
resource 68C0/69A0 also change palette/LUT state. Either retain the original
address-selected state as authoritative and read it at its native consumption
boundary, or observe these actual writers. A private callback-only texture
mirror is insufficient; a whole exclusive VRAM bank has not been proven.

Thus the minimum supported architecture has a shared early callback clone,
route-specific geometry provenance observers, and coordinated final-display
ownership. The exact complete presentation replacement hook set is still
unresolved. A local DDRAW module solves loading, not those other contracts.

## Recommended shared boundary

Use a persistent **250-word callback-table clone**, published through host
pointer cell RVA 0x365A0 before the first cold PC_DLLEngineMain call. Preserve
every unselected word, including shared data pointers. Each original engine
copies the prefix once into its own storage; publishing later does not update
an engine already initialized. This table interception avoids instruction detours for the selected host
callbacks and permits unchanged original EXE and engine files.

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

An entry snapshot records the initial stream. The [bounded callback fixture](live_capture.md)
shows that it can differ from the subsequently consumed stream while remaining
passive: a synthetic callback changes the current link or a future E1 payload
before AC60 reloads it. A replacement must dispatch from live storage and retain
the intervening native state and callback boundaries. This fixture identifies
no shipped callback that makes those changes.

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

The [surface contract](surface_contract.md) requires reacquiring the published
view after locking: unlock clears the lock byte but leaves pointer and pitch
stale. The [dirty-latch cases](dirty_gate.md) also require preserving the existing
presentation request and reproducing route-specific sets. Neither changed-pixel
counts nor a universal positive-area test reproduces those sets. Keep the
original presenter as the consumer of that request.

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

The direct graphics-init path is `0x7937 -> 0x5e10 -> 0x72a0`; it reaches
palette setup and calls the `DirectDrawCreate` thunk at `0x7426`. The separate
event/tick path `0x79a0 -> 0x79fc -> 0x5f20 -> 0x5ec0` can, behind its gates,
read the current table pointer at `0x5ef3` and dispatch through it at `0x5f03`
without passing through `DirectDrawCreate`. Its invocation timing and possible
reentrancy are unresolved, so the direct init order is not a process-wide
guarantee that DirectDraw runs before the first engine-table copy.

The host tests the `DirectDrawCreate` HRESULT at `0x742b` only to select error
logging; both branches rejoin and continue setup. A candidate must not publish
the table clone only on success. The smallest static candidate remains a
persistent data-only clone of all 250 DWORDs, with only proven callback words
replaced and every other word retained, published through `0x365a0` before
the first possible engine dispatch. The later bounded original-host diagnostic
observes an app-local diagnostic DLL publishing before EXE entry on the tested
machine. Forwarding and actual engine-copy visibility remain unobserved. The import-
resolution-only surrogate does not call the export, publish a clone, or run
Hercules. See
[bootstrap order](bootstrap_order.md), its [evidence](evidence/bootstrap_order.json), [loader feasibility](loader.md), and the separate [early-publication fixture](fixtures/early_loader_probe/README.md) with its [receipt](evidence/early_loader_surrogate.json).

Any future proxy must preserve the original DirectDraw arguments, returned
HRESULT, and output object. Keep `DllMain` to minimal initialization: Windows
calls it under the loader lock, and Microsoft advises against `LoadLibrary`
or operations that can acquire that lock from there. Resolve/forward the real
system implementation from a safe export-call path rather than doing complex
loading in `DllMain`. This is design guidance, not an implemented proxy.
[Microsoft: Dynamic-Link Library Best Practices](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-best-practices).

The proxy loads interception; DirectDraw itself is not the packet interface.

The [startup import audit](startup_import.md) now supports a conditional earlier
boundary. The original EXE has an ordinary static DDRAW import and no EXE TLS
callbacks, CLR header or delay import. If normal startup selects the candidate
module and its process attach succeeds, attach-time publication precedes host
entry, including the independent event path. This is a documented-loader
inference from the pinned PE. The later [pre-entry diagnostic](preentry_publication.md)
adds target deployment evidence for one isolated byte-identical EXE on the
tested Windows machine. It stops before EXE entry and observes no rendering. The
[coherence audit](interface_publication.md) also supports copying the table's
static words while preserving word 1 as a live shared-context pointer.

| Approach | Original file changes | Scope and reliability | Cross-module and presentation cost |
|---|---|---|---|
| Direct AC60 detour | None on disk; runtime instructions changed | Single shared host target, but requires safe instruction interception and complete outer effects | Covers three module submit routes; misses movie and other presentation paths |
| Selected callback clone | None; runtime data pointer changed | Smallest observed renderer seam; must precede cold copies and preserve borrowed state | Shared across three modules; TITLE extra upload/movie routes require explicit handling |
| Forwarding `ddraw.dll` bootstrap + clone | Add local proxy; originals intact | Diagnostic sibling selection and clone publication before EXE entry observed; deferred API-call publication can be bypassed by the independent path; HRESULT is logging-only | Original window/COM services still require forwarding; engine execution and presentation coexistence are unobserved |
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
at the initial clone boundary. Bootstrap publication was observed at the
pre-entry stop; window coexistence has not been run.

The [D60C feedback witness](packed_uv_recurrence.md) requires the sampled
texture byte to participate in later coordinate progression. A faithful backend
must reproduce that dependency; conventional interpolated UV alone omits it.
The [tested CC50/E680 paths](route_uv_feedback.md) have separate bounded
contracts and must not inherit D60C's recurrence.
