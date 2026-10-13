# Hercules PC renderer contract

This package establishes the original renderer boundary for a separate
`hercules_renderer` project. It contains raw x86 interface views, checked packet
decoding, bounded frame traversal, texture addressing and synthetic fixtures.
Diagnostic fixtures can publish a table clone in bounded probes. The package
contains no GPU backend or replacement gameplay module.

The current investigation prioritizes upstream geometry and complete display
ownership for true native-resolution output. [Native-resolution readiness](native_resolution.md)
separates geometry, association, viewport, composition, presentation and temporal
requirements from the existing screen-space and pixel-fidelity contracts.

The narrowest shared final-packet boundary is callback **89 / +0x164**, host RVA
**0xAC60**. ENG1, ENG3 and TITLE independently copy the same 250-DWORD host
interface. A persistent data-only clone published through host pointer cell
RVA `0x365a0` before the first possible engine dispatch is the smallest
interception candidate. A bounded original-host diagnostic now observes its
publication before the EXE entry instruction on the tested Windows machine;
renderer sufficiency remains unobserved.
The direct init chain calls `DirectDrawCreate` at RVA `0x7426` before its
later table-pointer read, but a separate guarded event/tick chain can reach
that read and dispatch without traversing `DirectDrawCreate`. There is no
process-wide ordering proof from those host paths alone. The later
[startup import audit](startup_import.md) supports conditional attach-before-host-entry
publication when ordinary startup selects the intended static DDRAW module and
its attach succeeds. The [pre-entry diagnostic](preentry_publication.md)
observes target selection and publication in an isolated byte-identical EXE.

The import-resolution surrogate receipt establishes sibling-DLL selection
for an ordinary static import on the tested machine; it does not call
`DirectDrawCreate` or publish a table clone. A separate [early-publication
fixture](fixtures/early_loader_probe/README.md) and its
[receipt](evidence/early_loader_surrogate.json) test synthetic table
publication. Neither fixture launches Hercules or establishes target
deployment behavior. The later original-host pre-entry diagnostic closes that
specific deployment question. No product proxy, engine execution or rendering
integration has been observed.

Read [interception.md](interception.md) before implementing a replacement.
AC60 performs state updates, VRAM moves, surface management, color remapping,
presentation and notification as well as drawing. A pure polygon renderer does
not replace this complete contract.

## Package map

| File | Purpose |
|---|---|
| hercules_projection.h / .c | Fixed-width single-point reference, joined views and x86 callback signatures |
| upstream_projection.md | Native arithmetic, passive observer, precision and alias limits |
| eng1_geometry_association.md | Original producer vertex-to-packet fixtures and required face observers |
| cross_module_geometry.md | Independent ENG3/TITLE batch and fourth-point paths |
| screen_ownership.md | Visible writers versus texture/palette/surface ownership |
| presentation_takeover.md | Forwarding diagnostic and remaining final-display ownership |
| temporal_state.md | Native camera history and interpolation prerequisites |
| native_resolution.md | Separate capability readiness and 4:3/widescreen limitations |
| hercules_host_abi.h | Raw copied prefix, verified slot constants, selected clone helper |
| hercules_packets.h | Observed wire layouts and checked packet decoder |
| hercules_frame.h | Module-specific OT descriptors and bounded raw-address walker |
| hercules_textures.h | Rectangle layout and measured addressing helpers |
| hercules_state.h | E1, F3 and command-01 bounded state decoders |
| hercules_surface.h | Raw published surface cells and argumentless service adapter types |
| hercules_staging.h | Persistent AC60 cells, suppression gate and operation descriptors |
| hercules_sprite_geometry.h | Sprite clipping/setup without pixel rasterization |
| hercules_quad_dispatch.h | Static rectangle route predicates |
| hercules_pixels.h | Bounded indexed shade/blend, palette and fill reference steps |
| hercules_packed_uv.h | Original-backed D60C packed register recurrence |
| hercules_route_uv_models.h | Separate CC50 and E680 raw-register transitions |
| hercules_edge_steps.h | Fixed-width reciprocal and guarded edge-step arithmetic |
| closure.md | Bounded host renderer call/data closure |
| ownership.md | Direct-reference locality versus shared host surface state |
| packets.md | Consumer fields versus independently observed producer extents |
| textures.md | Upload, page selection, VRAM move, indexed color and unresolved bounds |
| texture_ownership.md | PE backing envelopes and renderer-state overlap |
| palette.md | Startup LUT writer order, aliases and palette lifecycle |
| sprite_geometry.md | Signed clipping and wide UV addressing |
| polygon_dispatch.md | Native row-zero and rectangle discriminators |
| polygon_edges.md | Finite coverage, fixed-point steps and a wire-valid interpolation counterexample |
| shaded_edges.md | Neutral-red shade bypass, blended geometry and zero-source tests |
| quad_interpolation.md | Four-edge interpolation versus triangle splits |
| fill_edges.md | Constant-fill division, coverage and zero-byte behavior |
| reciprocal_domain.md | Native 2047/2048 guards and pre-crop reciprocal lookups |
| packed_uv_counterexample.md | Packed carry falsifies independent Q16 pixel accumulation |
| notification_target.md | Setter ABI, five direct NULL registrations and unresolved aliases |
| producer_reachability.md | TITLE mode-7 computed FT4 tag/link/stride proof |
| upload_domain_bounds.md | TITLE same-context y=256 and ENG3 script-derived input bounds |
| module_packets.md | Separate ENG3/TITLE producers and 0x68 semantics |
| eng3_handlers.md | Six-entry render dispatch frontier and FT4 writers |
| title_handlers.md | Child dispatch, grouped 0x68 records and FT4 mode writers |
| pixels.md | Distinct indexed pixel paths and their proof boundaries |
| frame_lifetime.md | Per-module frame/arena requests, reset, cursor selection and release frontiers |
| surface_contract.md | Bounded lock/unlock/presentation side effects and CPU writeback contract |
| dirty_gate.md | Route-specific presentation requests, including zero-store cases |
| route_uv_models.md | Native/C replay boundaries and geometry-derived pointer correction |
| frame.md | Initialization, prepend order, traversal and module lifecycles |
| notification.md | Primary/secondary callback ownership and live-link traversal |
| state.md | State masks, clip clamps and embedded VRAM-move rectangle |
| staging.md | Native effective staging and finite differential boundaries |
| compatibility.md | Independent ENG1/ENG3/TITLE callback evidence |
| presentation.md | Original window, DIB, DirectDraw, palette and movie paths |
| loader.md | Direct-init and independent dispatch paths; source-built loader surrogates |
| geometry.md | Information available before and after projection |
| blockers.md | Separate capture, rendering, window and enhancement blockers |
| callback_metadata.md | CMainFrame message-map ownership and the unowned event-callback cluster |
| interface_publication.md | Table-content coherence and the shared mutable context pointer |
| startup_import.md | Conditional attach-before-host-entry inference from original PE metadata |
| preentry_publication.md | Original-host sibling-DLL selection and data-only clone publication before entry |
| live_capture.md | Native callback counterexamples to entry-snapshot consumption equivalence |
| consumer_coverage.md | Dispatch-to-decoder coverage and unknown group-0x60 state effects |
| bootstrap_gates.md | Initial zero-fill and normal export publication order; indirect gate-setter limits |
| `bootstrap_order.md` | Direct-init and independent event/tick dispatch paths; HRESULT branch and ordering limits |
| `packed_uv_recurrence.md` | One finite D60C fixture where sampled texels change a later V byte |
| `route_uv_feedback.md` | Finite sample-address comparisons for CC50 and selected E680 modes |
| `texture_row_mapping.md` | Entry-latched A710/A8F0 row mapping, merged helper, and thirteen finite native cases |
| `fixtures/early_loader_probe/README.md` | Synthetic early-versus-deferred table publication fixture; no Hercules integration |
| evidence/ | Compact original identities, spans and provenance; no binary code |
| tests/ | Source-only synthetic and optional bounded original-code checks |

The C headers and utilities have no dependency on a GPU API or the game source
graph. Repository verification scripts reuse the existing PE/toolchain tools.
Copy this folder to reuse the C utilities. Its standalone synthetic runner
needs only Python 3 and caller-supplied C/C++ compilers:

```powershell
python renderer_contract/tests/run_portable_tests.py --cc gcc --cxx g++ --output work/renderer_portable
```

Use a new or empty output directory outside the package. The runner records
compiler identities, source hashes and each fixture result there. GCC builds
use strict C89/C++98 modes; MSVC uses `/Za` and the corresponding language
subset. On x64 these fixtures validate raw 32-bit address views; live callback
declarations additionally require x86. The pinned VC5 and original-code gates
below remain repository-specific and separate from portable utility tests.

The existing partial typed declaration in
[calibration/engine_interface.h](../calibration/engine_interface.h) remains the
repository's typed interface view. This package treats its observed prefix as
raw fixed-width words; it does not create a competing EngineInterface struct.
Packet addresses are 32-bit tokens, resolved through an explicit reader, never
cast to native pointers by offline helpers. Live callback adapters require an
x86 process and caller-cleaned stack arguments.

## Proof and reproducibility

`STATIC_OBSERVED` means instruction/data evidence in hash-pinned PC originals.
`LAYOUT_VERIFIED` adds compiled size/offset assertions and synthetic tests.
`BOUNDED_EQUIVALENT` applies only to named finite native-code fixtures.
`HOST_INTEGRATION_OBSERVED` requires a recorded actual host observation.
`UNRESOLVED` marks unsupported assumptions or missing evidence.

No renderer interception or GPU output has HOST_INTEGRATION_OBSERVED status.
No new FUNCTION_MATCH claims are made and recovery.json is unchanged.
Fixtures are finite evidence, not proof of universal rendering equivalence.

From the repository root:

```powershell
python renderer_contract/tests/run_tests.py
python renderer_contract/tests/validate_originals.py --original-root D:\Prog\hercules_recon
```

Optional CPU-only original-code checks require the pinned Unicorn runtime in
evidence/oracle_toolchain.json (version, official origin and hashes):

See [verification.md](verification.md) for shipped native verification commands.

These map original bytes in a CPU emulator. AC60 tests record rasterizer and
VRAM-move call boundaries while skipping those callees; they prove staging and
dispatch under that fixture, not rasterizer output. C882 separately tests a
finite shade/blend loop. No DirectDraw device or game session is exercised.

Generated test executables and receipts stay under ignored work/. Original
binary paths may be supplied explicitly; nothing is written to assets/.
The executable oracle is the unpatched installation copy. The user's no-CD
variant has identical recorded renderer/table/loader spans and one differing
byte elsewhere; see [exe_variant.md](exe_variant.md). A future loader must
identify its supported executable before using these RVAs.

The generic packet decoder leaves command 01 and F3 as visibly unsupported
payloads. Their proven fields are decoded by hercules_state.h; command 01
requires 24 readable bytes including its embedded rectangle. The 16-byte
direct AC60 access minimum alone is insufficient for that helper.

The investigation starts from commit 73c6320179093c3de9b820e66b87f94dfb639a9b
on an isolated renderer branch. Detailed proof references accompany each
subsystem. These contracts are GPU API independent.

[Skewed quad fixtures](skewed_quads.md) verify five finite shapes across three
original routes. [Upload producer domains](texture_producer_domains.md) recover
the shared ENG1/ENG3 high-band startup rectangle and its exact native write range.

Current verification passes 19 suites with VC5 and 19 with GCC. Bounded C
replays match 1,029 D60C register transitions and 5,423 CC50/E680 transitions.
Thirteen native texture fixtures verify the named row/address cases; ten
surface cases and 26 dirty-latch cases cover their stated synthetic conditions.
These results do not establish whole-renderer equivalence or live Hercules
integration. See [verification.md](verification.md) for the exact proof scopes.

The [zero-argument batch observer](batch_projection.md) captures the first three
camera/screen lanes before a fourth single-point projection can overwrite them.
Slots 102/107/117 are now included in the selected callback clone API. The
[text-writer contract](gdi_text_writer.md) corrects the windowed DC acquisition
to `GetDC(NULL)` and leaves its runtime trigger and actual pixels unresolved.

[ENG3 direct FT4 association](eng3_geometry_association.md) and
[TITLE mode-5 association](evidence/title_geometry_association.md) execute their
original producers into matching packet corners and OT insertion. Both require
callback-time and vertex-role identity because one point pointer is reused.
They do not establish upstream visibility, whole-module geometry or pixels.
