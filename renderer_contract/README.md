# Hercules PC renderer contract

This package establishes the original renderer boundary for a separate
`hercules_renderer` project. It contains raw x86 interface views, checked packet
decoding, bounded frame traversal, texture addressing and synthetic fixtures.
It does not install hooks, implement a GPU renderer, or replace game modules.

The narrowest shared boundary observed is callback **89 / +0x164**, whose host
target is HERCULES.EXE RVA **0xAC60**. ENG1, ENG3 and TITLE independently copy the
same 250-DWORD host interface. Redirecting selected words in a persistent clone
before each module's first copy can retain all original engine binaries.
This is a static feasibility conclusion; installation has not been observed.

Read [interception.md](interception.md) before implementing a replacement.
AC60 performs state updates, VRAM moves, surface management, color remapping,
presentation and notification as well as drawing. A pure polygon renderer does
not replace this complete contract.

## Package map

| File | Purpose |
|---|---|
| hercules_host_abi.h | Raw copied prefix, verified slot constants, selected clone helper |
| hercules_packets.h | Observed wire layouts and checked packet decoder |
| hercules_frame.h | Module-specific OT descriptors and bounded raw-address walker |
| hercules_textures.h | Rectangle layout and measured addressing helpers |
| hercules_state.h | E1, F3 and command-01 bounded state decoders |
| hercules_staging.h | Persistent AC60 cells, suppression gate and operation descriptors |
| hercules_sprite_geometry.h | Sprite clipping/setup without pixel rasterization |
| hercules_quad_dispatch.h | Static rectangle route predicates |
| hercules_pixels.h | Bounded indexed shade/blend, palette and fill reference steps |
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
| frame.md | Initialization, prepend order, traversal and module lifecycles |
| notification.md | Primary/secondary callback ownership and live-link traversal |
| state.md | State masks, clip clamps and embedded VRAM-move rectangle |
| staging.md | Native effective staging and finite differential boundaries |
| compatibility.md | Independent ENG1/ENG3/TITLE callback evidence |
| presentation.md | Original window, DIB, DirectDraw, palette and movie paths |
| loader.md | Pre-copy bootstrap timing and source-built loader surrogate |
| geometry.md | Information available before and after projection |
| blockers.md | Separate capture, rendering, window and enhancement blockers |
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
