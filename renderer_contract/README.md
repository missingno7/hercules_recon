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
| closure.md | Bounded host renderer call/data closure |
| packets.md | Consumer fields versus independently observed producer extents |
| textures.md | Upload, page selection, VRAM move, indexed color and unresolved bounds |
| palette.md | Startup LUT writer order, aliases and palette lifecycle |
| sprite_geometry.md | Signed clipping and wide UV addressing |
| polygon_dispatch.md | Native row-zero and rectangle discriminators |
| module_packets.md | Separate ENG3/TITLE producers and 0x68 semantics |
| pixels.md | Distinct indexed pixel paths and their proof boundaries |
| frame.md | Initialization, prepend order, traversal and module lifecycles |
| notification.md | Primary/secondary callback ownership and live-link traversal |
| state.md | State masks, clip clamps and embedded VRAM-move rectangle |
| staging.md | Native effective staging and finite differential boundaries |
| compatibility.md | Independent ENG1/ENG3/TITLE callback evidence |
| presentation.md | Original window, DIB, DirectDraw, palette and movie paths |
| geometry.md | Information available before and after projection |
| blockers.md | Separate capture, rendering, window and enhancement blockers |
| evidence/ | Compact original identities, spans and provenance; no binary code |
| tests/ | Source-only synthetic and optional bounded original-code checks |

The C headers and utilities have no dependency on a GPU API or the game source
graph. Repository verification scripts reuse the existing PE/toolchain tools.

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
