# Native-resolution rendering and display ownership

The target is a future drop-in renderer that preserves original simulation
while controlling the complete visible image at the output resolution. This
package establishes its inputs and compatibility boundaries; it implements no
GPU backend or interpolation.

## Distinct readiness boundaries

| Capability | Established foundation | Material missing proof |
|---|---|---|
| Passive interception prototype | Original-host pre-entry DDRAW selection/table publication; cold-prefix copies in all three DLLs; source-built synthetic clone/forward smoke and same-process real-system create/release diagnostic | Transparent forwarding in original Hercules; actual engine first-copy, lifetime and frame observations |
| Faithful screen-space GPU rendering | Ordered packet layouts, indexed state and finite original pixel/edge fixtures | Complete reachable producer/command coverage and display composition ownership; no GPU implementation here |
| Genuine upstream geometry | Single-point original/C/compiled observer: 14 cases, 632 assertions; batch zero-argument observer: three cases plus a recorded trap | Live passive capture, all producer families and upstream units/material ownership |
| Cross-module vertex association | Original ENG1 GT4/direct/observer/culled-prefix and FT4 tail; ENG3 direct FT4; TITLE mode-5 FT4 in two contexts | Runtime producer observers, production bounds/lifetimes, upstream visibility and complete mesh coverage |
| Higher-resolution 4:3 viewport | Distinct retained camera X values collapse to one native screen WORD; private rational reprojection can retain the distinction | Association/culling completeness, output composition and native-resolution presentation |
| Wider world view | Upstream points and engine visibility gates located | Geometry discarded inside engines, view extension and camera/level limits; final packets cannot restore it |
| Complete image composition | Indexed OT content, direct texture/LUT/palette writers, movie, GDI text, clears and presentation are separately mapped | Chronological complete writer coverage; slot149 trigger and framework/computed routes remain unresolved |
| Independent presentation | HWND/surface lifecycle contract; guarded source-built system API forwarding | Exclusive display ownership, mode/window changes, loss/restoration, synchronization and shutdown |
| Temporal interpolation | Two controlled native camera-history calls; camera/actor/pool state locations | Full update cadence, stable object identity, reuse/discontinuities and animation/pose lifetime |

These are separate readiness boundaries, not completion percentages. The
passive prototype has an evidence-backed foundation; high-resolution geometry
is feasible for the measured path but not ready as a complete game renderer.
Complete composition, independent presentation, widescreen visibility and
interpolation remain unresolved.

See [projection](upstream_projection.md),
[ENG1 association](eng1_geometry_association.md),
[ENG3/TITLE paths](cross_module_geometry.md),
[screen ownership](screen_ownership.md),
[presentation takeover](presentation_takeover.md) and
[temporal state](temporal_state.md).

## Resolution does not recover discarded information

Final FT3/FT4/GT4 routes consume projected coordinates without per-vertex Z,
reciprocal depth or normals. Multiplying those coordinates scales their existing
quantization. Higher-resolution rasterization can improve polygon edge coverage,
but actual upstream reprojection needs the earlier point and transform outputs
plus proven associations with each emitted vertex.

The single-point projection callback is a bounded-tested observation boundary. Call it
once in its existing place, then read its still-live camera and screen outputs
into private storage. Do not introduce extra snapshot/transform calls. Observe
the point-input and projected-output destinations where needed to establish
provenance; matching coordinates alone are insufficient when points repeat.
For ENG1, callback-only capture loses selected face-index provenance. Additional
pre-face observers at `2E0F1` / `2E27B`, confirmed by actual packet emission,
are needed. ENG3/TITLE use the distinct three-point `102/107/117` sequence
before projecting the fourth point; slot106 alone misses those first lanes.

Keep original projection and culling results available to simulation. Private
rendering resolution must not change the host's dimensions, gameplay state,
collision calculations or update cadence. A wider aspect ratio is a separate
visibility problem: a backend cannot draw geometry discarded before capture.
Ordinary depth testing or perspective-correct interpolation would be an
enhancement requiring additional data and policy, not an inferred native rule.

## Display control requires a broader boundary

The original presenter consumes a 320x240 indexed DIB or DirectDraw surface.
Copying a modern-resolution scene back into that fixed-size output cannot
retain its native-resolution samples. Retaining that presenter remains useful
for compatibility experiments, but independent native-resolution presentation
requires additional ownership beyond submission slot 89.

| Architecture | Preserved original operations | Required control and limitation |
|---|---|---|
| Original presenter | Window, palette, movies, surface and present services | Consumes the original 320x240 image; cannot establish native-resolution scene output |
| GPU output copied to original surface | Original CPU view, remap, dirty state and presenter | Must respect actual locked pitch; fixed image dimensions remain a resolution ceiling |
| GPU presentation through original HWND | Original window/input owner where compatible | Must coordinate every GDI/DirectDraw/movie/palette writer and mode transition; not implemented or verified |
| Ordered indexed/geometry composition | Original state-changing commands and synchronous notifications | Needs producer provenance, exact order, live texture/LUT state and independent final presentation |

The original framebuffer already combines world and screen-space content.
Treating it as one opaque overlay or background loses some intended ordering.
Use proven command and writer order rather than inventing persistent layers.
Textures, palette tables and visible pixels are distinct writer classes, with
actual aliases; a callback-only private texture mirror misses direct host
calls. See [palette](palette.md), [texture ownership](texture_ownership.md),
[surface contract](surface_contract.md) and [live callback traversal](live_capture.md).

Borrowed pointers cannot become asynchronous GPU work without an ownership or
copy protocol. State changes, VRAM moves and notifications must retain their
native synchronous positions; a whole-frame entry snapshot is not a substitute
for the subsequently consumed stream.

## Current evidence boundary

The starting baseline is contract commit `c5273be`. Revalidation checks four
original images, 634 original-code spans and 72 evidence files. These checks
preserve earlier finite proofs; they do not establish new geometry, display
takeover or temporal behavior. The Phase14 additions are separate finite projection/association/camera proofs,
static module/writer audits and source-built forwarding observations. They do
not relabel historical pixel fixtures or establish original-game display
integration. Original-host pre-entry evidence remains tied to its earlier
helper revision; the enlarged clone helper has separate synthetic verification.

Phase15 and Phase16 add the actual zero-argument batch capture boundary and
finite original ENG3/TITLE point-to-packet execution. First-three camera lanes
must be captured after slot107 returns and before the fourth-point slot106
overwrites the host lanes. ENG3/TITLE reuse a local point pointer within the
primitive; retain role and callback-time provenance. This removes a selected-path
association uncertainty, not complete visibility or native-resolution display
ownership. The slot149 windowed writer requests `GetDC(NULL)` and releases
using the current HWND, so client-window overlay ownership cannot be assumed.

The [selected ENG3 visibility trace](fixtures/viewport_visibility/viewport_visibility.md)
finds indexed coordinate bounds and depth gates before emission, with no
projected-XY rectangle test in those predicates. It does not identify the
bounds' meaning or a runtime center of 160/120. The
[movie request/copy trace](movie_writer.md) identifies the host-owned decoded
indexed buffer, origin-zero pitched copy and palette clear/present order.
Neither result closes wider-view policy or complete composition serialization.
