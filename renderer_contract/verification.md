# Verification boundaries

## Current native-resolution checkpoint

Fresh public-path verification passes 19 VC5 and 19 strict GCC C/C++ suites.
The single-point C model and compiled observer pass 14 original-x86 cases /
632 assertions. The new module audit checks 60 spans and 27 wrappers; its
bounded identity fixture executes host callbacks only, not ENG3/TITLE.
The screen audit checks 20 host spans and eight selected callback cells.
Two controlled original camera calls execute 64 instructions without external
calls; these are not two full simulation updates. Original ENG1 association
has a separate producer receipt, with synthetic mesh inputs rather than a
live scene or asset-based integration claim.

The source-built presentation fixture passes synthetic ABI/recursion guards
and a same-process system DirectDrawCreate/Release observation. It does not
start Hercules, initialize an engine or own a display. Its first raw path
failure and later cross-process metadata followup remain unchanged; the fresh
same-process result is a distinct receipt. Earlier original-host pre-entry
publication remains tied to its pinned historical helper revision.

The append-only manifest records exact source, image/span and receipt hashes.
`STATIC_OBSERVED`, `LAYOUT_VERIFIED`, `BOUNDED_EQUIVALENT` and live host
observations remain distinct. No new `FUNCTION_MATCH` or recovery entry is
created. Details: [projection](upstream_projection.md),
[association](eng1_geometry_association.md),
[module paths](cross_module_geometry.md), [writers](screen_ownership.md),
[presentation](presentation_takeover.md), [temporal](temporal_state.md).

## Historical fixtures and limits

The contract starts from original PC instructions, not a shared reconstructed
software-rasterizer model. Original-image hashes and bounded RVA span hashes
are checked by tests/validate_originals.py. This catches changed inputs, not
errors in semantic interpretation; the evidence also records producer/consumer
offsets and callback bindings.

The C suite compiles fresh with pinned VC5. Size/offset assertions and explicit
byte fixtures exercise packet fields, truncation/unknown statuses, clone
validation with unchanged output on rejection, and bounded OT traversal. Raw
addresses are resolved into synthetic mapped regions. Decoder range/cycle
guards are intentional tool safeguards absent from the original AC60.

The optional native CPU oracle maps original PE sections at their preferred
address inside Unicorn, with synthetic arguments, stack and notification
state. Target instructions remain unchanged. Unknown endpoints, OS calls,
unmapped memory and instruction-budget exhaustion fail. These tests execute
the original instruction region and compare its observed writes/call boundary
with explicit expected values. No original code is embedded in this package.

A710's finite native tests corrected the preliminary y-row bank interpretation.
That correction is reflected in the address helper. It demonstrates why
synthetic tests of a shared formula alone do not establish behavioral accuracy.

AC20's existing strict FUNCTION_MATCH evidence is reused from recovery.json;
this work does not promote functions or recompute recovery counts. Existing
canonical source and strict evidence stay outside the renderer changes.

Historical scope at this earlier checkpoint: host loading, interception, GPU pixels and window coexistence were unobserved. The later pre-entry publication proof is linked above; rendering/window coexistence remain unobserved.
No successful game launch or rendering replacement is claimed. Native CPU
emulation is distinct from HOST_INTEGRATION_OBSERVED.

The current checkpoint passes twelve VC5 C/C++ suites and twelve portable
GCC64 suites. The new fixed-width edge-step fixture has 27 checks. Numeric fixtures
include 44 frame, 43 packet, 43 state, 47 indexed-pixel, 24 sprite-pixel and
74 staging checks. Native fixtures add 26 sprite setup cases, eight pixel
slices, nine VRAM/alias cases, six row/rectangle cases, twelve DLL prefixes
and one complete bounded startup LUT writer. The staging differential checks
171 native fields, seven gate branches and nine page-mask vectors. Original
validation covers four image hashes, 473 spans, 29 callback bindings,
22 wrappers and 57 dispatch sites. Counts describe finite evidence only.

Additional geometry fixtures cover 22 triangle routes, 17 shaded/raw blend
cases, three generic-quad routes and seven constant-fill cases with six paired
comparisons. They falsify exact barycentric sampling, both tested triangle
splits for one quad, and unified fill/textured edge arithmetic. These are
model counterexamples, not defects in the original game.

The original palette builder falsified an earlier interpretation of its
CLUT writer as a clear: it writes byte ramps. It also showed that selector 3
adds column to half-row before saturation. The published formula includes
that correction and the later 192-byte overwrite of blend selector 0.

## Reproduce from the repository root

The reciprocal-domain runner adds twelve finite original-code cases and
fifteen assertions. Complete-section candidate inventories explicitly record
skipdata bytes and partial function anchors; they do not prove absence of
computed aliases or universal instruction alignment. The loader surrogate
observes source-built x86 import selection only. No original host, DirectDraw
device or GPU integration is inferred from it.

Use Python 3 with the existing repository PE dependencies. On this machine:

```powershell
$pythonExe = 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
$originalExe = 'D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE'
& $pythonExe renderer_contract/tests/run_tests.py
& $pythonExe renderer_contract/tests/validate_originals.py --original-root D:\Prog\hercules_recon
& $pythonExe renderer_contract/tests/test_original_pixel_slices.py --exe $originalExe --receipt work/replay/pixels.json
& $pythonExe renderer_contract/tests/test_original_polygon_dispatch.py --exe $originalExe --receipt work/replay/polygons.json
& $pythonExe renderer_contract/tests/test_original_sprite_geometry.py --exe $originalExe --receipt work/replay/sprites.json
& $pythonExe renderer_contract/tests/test_original_vram_move.py --exe $originalExe --receipt work/replay/moves.json
& $pythonExe renderer_contract/tests/test_original_palette_builder.py --repo-root . --exe $originalExe --receipt work/replay/palette.json
& $pythonExe renderer_contract/tests/test_original_startup_pixel_alias.py --exe $originalExe --receipt work/replay/alias.json
& $pythonExe renderer_contract/tests/test_original_polygon_edges.py --exe $originalExe --receipt work/replay/edges.json
& $pythonExe renderer_contract/tests/test_original_shaded_edges.py --exe $originalExe --receipt work/replay/shaded.json
& $pythonExe renderer_contract/tests/test_original_quad_interpolation.py --exe $originalExe --receipt work/replay/quad.json
& $pythonExe renderer_contract/tests/test_original_fill_edges.py --exe $originalExe --receipt work/replay/fill_full.json --compact-receipt work/replay/fill.json
& $pythonExe renderer_contract/tests/test_original_reciprocal_domain.py --exe $originalExe --receipt work/replay/reciprocal.json
& $pythonExe renderer_contract/tests/audit_state_ownership.py --original-root . --output work/replay/ownership.json
& $pythonExe renderer_contract/tests/audit_texture_ownership.py --original-root . --exe $originalExe --output work/replay/texture_backing.json
& $pythonExe renderer_contract/tests/audit_loader.py --original-root . --exe $originalExe --output work/replay/loader.json
& $pythonExe renderer_contract/tests/audit_eng3_handlers.py --receipt work/replay/eng3_handlers.json
& $pythonExe renderer_contract/tests/audit_title_handlers.py --output work/replay/title_handlers.json
& $pythonExe renderer_contract/tests/test_native_staging.py --exe $originalExe --receipt work/replay/staging.json
& $pythonExe renderer_contract/tests/test_original_interface_copy.py --exe $originalExe `
  --eng1 D:\Prog\hercules_recon\assets\pc\HERCULES\ENG1.DLL `
  --eng3 D:\Prog\hercules_recon\assets\pc\HERCULES\ENG3.DLL `
  --title D:\Prog\hercules_recon\assets\pc\HERCULES\TITLE.DLL `
  --receipt work/replay/interface.json
```

All original paths are explicit and read-only. Supply your own original paths
for another installation. The oracle hashes intentionally reject unsupported
images, including the differing no-CD EXE; the variant comparison is separate.
Native tests default to pinned Unicorn and analysis dependencies under C:\tools.
The staging differential invokes the common C runner as well as three CPU
fixtures. Receipts and compiler products are confined to ignored work/.
Windows can reject the old compiler in the sandbox with error 623; the same
workspace-only compile/run outside that sandbox succeeds. No tools are patched.
The quad test filename avoids legacy installer detection of the word "patch"
inside "dispatch", which otherwise requested elevation for that test executable.

Luna xhigh workers performed the bounded call/field audits, module comparison,
frame/texture work, native CPU harnesses and reference utilities. The integrator
selected the clone boundary, resolved conflicting LEA/field/address evidence,
kept snapshot capture from mutating geometry state, and ran the package gates.

The package now has thirteen VC5 and thirteen portable synthetic suites.
Additional unchanged-original fixtures cover five skewed quads across three
routes (15 runs) and one 128-KiB high-band upload. They establish finite
coverage/UV streams and write addresses; allocation ownership and resource
rectangle bounds remain unresolved. Reproduce from the renderer checkout:

```powershell
& $pythonExe renderer_contract/tests/test_original_skewed_quads.py --original-root . --exe $originalExe --receipt work/replay/skewed/full.json
& $pythonExe renderer_contract/tests/audit_upload_producers.py --original-root . --exe $originalExe --image-dir D:\Prog\hercules_recon\assets\pc\HERCULES --output work/replay/upload_sites.json
& $pythonExe renderer_contract/tests/test_original_cold_upload.py --original-root . --exe $originalExe --output work/replay/cold_upload.json
```

The later packed-UV fixture confirms a finite model counterexample, rather
than accepting the proposed separate-component iterator. Two original runs
each capture 36 samples; packed U carry produces 151 where the model predicts
150. Notification, producer and upload-domain audits add static gates and
explicit original span pins. None observes live host integration.

```powershell
& $pythonExe renderer_contract/tests/test_original_packed_uv.py --original-root . --exe $originalExe --receipt work/replay/packed_uv.json
& $pythonExe renderer_contract/tests/audit_callback_target.py --original-root D:\Prog\hercules_recon --exe $originalExe --output work/replay/notification.json
& $pythonExe renderer_contract/tests/audit_producer_reachability.py --title D:\Prog\hercules_recon\assets\pc\HERCULES\TITLE.DLL --exe $originalExe --output work/replay/producer.json
& $pythonExe renderer_contract/tests/audit_upload_domain_bounds.py --original-root . --exe $originalExe --image-dir D:\Prog\hercules_recon\assets\pc\HERCULES --output work/replay/upload_domains.json
```

The next bounded fixtures separate D60C's sampled-texel feedback from the
tested CC50/E680 color paths. A C89 state utility reproduces 1,029 captured
sample transitions across 15 spans with both VC5 and GCC; this does not validate
span construction or another raster route. Thirteen texture fixtures establish
entry-latched row mapping and full DWORD destination arguments. The static
bootstrap audit distinguishes the direct graphics-init path from an independent
guarded engine-dispatch path. Neither proves live host startup ordering.

```powershell
& $pythonExe renderer_contract/tests/test_original_packed_uv_feedback.py --original-root . --exe $originalExe --receipt work/replay/packed_feedback.json
& $pythonExe renderer_contract/tests/test_original_packed_uv_state.py --package renderer_contract --original-root . --exe $originalExe --receipt work/replay/packed_state.json --output work/replay/packed_state_build
& $pythonExe renderer_contract/tests/test_original_route_uv_feedback.py --original-root . --exe $originalExe --receipt work/replay/route_feedback.json
& $pythonExe renderer_contract/tests/test_original_texture_row_boundaries.py --original-root . --exe $originalExe --receipt work/replay/texture_rows.json
& $pythonExe renderer_contract/tests/audit_bootstrap_order.py --original-root . --exe $originalExe --output work/replay/bootstrap.json
& $pythonExe renderer_contract/fixtures/early_loader_probe/build_and_run.py --repo-root . --output-dir work/replay/early_loader_build
```

Build directories must be new or empty. The last command executes only a
source-built synthetic EXE and mock DLL: early attach publication reaches its
constructor's first copy, while deferred API publication misses it. Its HRESULT
and output are deliberately `E_NOTIMPL` and null. Hercules, real DirectDraw,
COM, windows and GPU services are outside that fixture.

```powershell
& $pythonExe renderer_contract/tests/audit_bootstrap_gates.py --original-root . --image-dir D:\Prog\hercules_recon\work\discs\pc_install --output work/replay/bootstrap_gates.json
```

This read-only audit checks all four pinned images and bounds the direct gate
writers and callback-address context. Its full raw-section sweep is a candidate
xref inventory, not proof of instruction alignment or callback scheduling.

The surface ABI and header additions pass fifteen fresh VC5 suites and fifteen
portable suites. Ten bounded original-x86 cases check lock, unlock and
presentation against synthetic COM/GDI services. They establish the observed
byte-sized mode reads, publication order and stale-view behavior; real windows,
drivers and surface-loss recovery remain outside the fixture.

```powershell
& $pythonExe renderer_contract/tests/test_original_surface_contract.py --repo-root . --exe $originalExe --output work/replay/surface.json
& $pythonExe renderer_contract/tests/audit_frame_lifetime.py --repo-root . --exe $originalExe --eng1 D:\Prog\hercules_recon\work\discs\pc_install\ENG1.DLL --eng3 D:\Prog\hercules_recon\work\discs\pc_install\ENG3.DLL --title D:\Prog\hercules_recon\work\discs\pc_install\TITLE.DLL --receipt work/replay/frame_lifetime.json
& $pythonExe renderer_contract/tests/audit_callback_metadata.py --repo-root . --image-dir D:\Prog\hercules_recon\work\discs\pc_install --output work/replay/callback_metadata.json
```

The latter two commands revalidate pinned static evidence. Frame allocation
requests and cursor associations do not establish safe producer capacities or
asynchronous ownership. CMainFrame message-map entries establish callback
metadata, while the separate RVA 0x79A0 cluster remains unowned.

The route-UV additions pass sixteen fresh VC5 suites and sixteen portable
suites, including a C++ consumer linked against the C implementation. Both
compilers reproduce 5,423 captured loop transitions and 6,380 lookup addresses
across CC50 and two E680 routes. A separate GCC differential reproduces 27
geometry-derived transitions with nonzero pointer corrections and one controlled
raw-boundary carry-out case. The raw case establishes no geometry reachability.

```powershell
& $pythonExe renderer_contract/tests/test_original_route_uv_state.py --package renderer_contract --exe $originalExe --original-root . --build-dir work/replay/route_uv_build --receipt work/replay/route_uv.json --vc5-toolchain C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953
& $pythonExe renderer_contract/tests/test_original_pointer_correction.py --package renderer_contract --exe $originalExe --original-root . --build-dir work/replay/pointer_build --receipt work/replay/pointer.json
& $pythonExe renderer_contract/tests/test_original_dirty_gate.py --repo-root . --exe $originalExe --output-dir work/replay/dirty_gate
```

Build directories must be new or empty. The last command executes 26 finite
original-x86 cases and a full file-backed `.text` candidate sweep. Accepted
transparent and some degenerate primitives still request presentation; 70C0
can rewrite 76,800 surface bytes without setting that request. These are
route-specific observations, not full raster or host integration equivalence.

Four bounded original-execution chains now test live callback timing. The
synthetic service rewrites a link, changes a future E1 payload, or disables
the notification gate; native AC60 consumes the changed storage after return.
These cases compare explicit trace expectations with original execution and
make no C implementation or shipped-callback mutation claim.

```powershell
& $pythonExe renderer_contract/tests/test_original_live_capture.py --original-root . --exe $originalExe --receipt work/replay/live_capture.json
& $pythonExe renderer_contract/tests/audit_interface_publication.py --repo-root . --exe $originalExe --eng1 D:\Prog\hercules_recon\work\discs\pc_install\ENG1.DLL --eng3 D:\Prog\hercules_recon\work\discs\pc_install\ENG3.DLL --title D:\Prog\hercules_recon\work\discs\pc_install\TITLE.DLL --output work/replay/interface_publication.json
& $pythonExe renderer_contract/tests/audit_import_preentry.py --original-root . --exe $originalExe --output work/replay/startup_import.json
```

The two static audits establish checked table-content coherence, the shared
context pointer, and ordinary static-import PE metadata. Attach-before-host-entry
timing is a conditional Windows-loader inference; neither audit observes the
candidate module being selected or installed in a Hercules process.

The unknown group-0x60 correction passes seventeen fresh VC5 suites and
seventeen GCC suites. Twelve bounded native cases establish the preamble's
page write and its byte-sized gate, including an adjacent-byte canary. The
new synthetic C suite checks its explicit unrecognized status and state event.
The existing native staging replay still matches 171 fields, seven gate
branches and nine masks against the corrected helper.

```powershell
& $pythonExe renderer_contract/tests/test_original_group60.py --original-root . --exe $originalExe --receipt work/replay/group60.json
& $pythonExe renderer_contract/tests/test_native_staging.py --original-root . --exe $originalExe --receipt work/replay/staging_regression.json
```

The latter command uses the pinned VC5 compiler. Both original-code runners
stop at bounded consumer/callee boundaries; they do not validate pixels,
positive-area VRAM moves, or the full host presentation path.

The [pre-entry fixture](fixtures/preentry_probe/README.md) separately compiles
a diagnostic DLL and debugger with pinned VC5, then runs normal and injected
failure tests against a source-built synthetic target. Its explicit
`--run-target` mode creates one isolated byte-identical original EXE process,
checks sibling DLL selection and table publication at an execution hardware
breakpoint on the entry instruction, then confirms termination and exit-event
draining. The [deployment receipt](evidence/preentry_deployment.json) is
**HOST_INTEGRATION_OBSERVED** only for that pre-entry boundary. It claims no
engine, window, DirectDraw forwarding, gameplay or GPU integration.

The [batch projection observer](batch_projection.md) uses the actual zero-argument
slot-107 ABI. Three finite original-x86 cases preserve raw EAX, stack, callee-saved
registers and native lane writes; an INT_MIN/-1 division trap is recorded as
non-success. Single-point projection (14 cases, 632 assertions) and ENG1
point-to-packet association (four producer fixtures) were replayed against the
expanded shared headers. [Phase15 regression evidence](evidence/phase15_regressions.json)
binds those fresh receipts to their tested sources. VC5 and GCC each pass 19 suites.

The [slot-149 text writer](gdi_text_writer.md) executes only its original bounded
body with controlled formatter/API/COM endpoints: five cases verify arguments,
failure flow and the one-byte counter, with no Windows drawing or pixel proof.
Its `GetDC(NULL)` finding supersedes the old screen-ownership JSON description;
the historical receipt remains byte-identical. Neither fixture establishes a
production hook, complete writer closure or original-game renderer integration.

The [ENG3 FT4 association](eng3_geometry_association.md) executes `231A0` directly
with synthetic inputs and matches four native point/camera/screen roles to packet
corners and OT insertion. The upstream handler visibility chain is not executed.
The [TITLE mode-5 fixture](evidence/title_geometry_association.md) passes two
resource contexts and 23 assertions, including eight independent positive-point
comparisons to the current C89 single-point model. These are bounded producer
proofs, not general batch arithmetic, runtime object identity or rendered pixels.
Both fixtures reject unknown code endpoints and preserve the original binaries.

The [movie writer](movie_writer.md) and [ENG3 visibility audit](fixtures/viewport_visibility/viewport_visibility.md)
add static-only contracts: host-owned movie decode/copy and palette ordering;
indexed parent-coordinate/depth rejection and child depth rejection before
emission. Fresh audits pin their originals and callback cells. They make no
playback, pixel, live visibility or full display-ownership claim.

The [three-point C89 reference](batch_reference.md) now passes direct comparison
with original B320 for five completed cases and one expected division fault,
including 83 ordered output stores and partial-result canaries. It retains
unclamped/zero-Z batch semantics separately from single-point projection.
The live callback ABI and the joined tool-view API remain distinct.
