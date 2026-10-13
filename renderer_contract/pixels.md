# Indexed pixel-path contract (private renderer evidence)

## Scope and result

This is a static reading of the original PC `HERCULES.EXE` plus one sibling finite original-byte oracle. Its SHA-256 is `587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d` at preferred image base `0x400000`. The C700/C882 color-LUT/blend slice has **BOUNDED_EQUIVALENT** evidence for four finite CPU fixtures; no D420/CC50, C8F0, CAB0, or complete raster entry was executed here. Claims below marked **STATIC_OBSERVED** describe bytes and direct control flow; the note about the embedded `0x92C490` load is a **STATIC_INFERENCE** from the entry's control flow. Remaining one-pixel cases are **PROPOSED_ONLY**.

The broad prediction that all four routes skip source byte zero was **PARTIALLY FALSIFIED**: the active CC50, C700, C8F0, and CAB0 pixel loops test the raw source byte and skip the write when it is zero. D420's gate-off, nonzero-row route instead passes the sampled byte to C2E0, which fills the clipped span without a zero test. D420's normalized-row-zero branch is a third path and is not characterized here as either CLUT or raw fill.

## Original entry and helper paths

| Entry/path | Direct route in original code | Indexed table behavior | Source-zero behavior |
|---|---|---|---|
| D420 `bit1_clear_polygon` (`0xD420..0xDD70`) | It normalizes row `r = SAR32([0xA90A04], 6) - 0x1E0`; values unsigned-greater than 30 become row 0. For `gate=[0x96E3E0] != 0` and `r != 0`, it calls CC50. For `gate == 0` and `r != 0`, it samples and calls C2E0. Row 0 jumps to D60C, outside these two routes. | Active CC50 setup: `palette = 0x92C390 + (r << 8)`; lookup index is the raw sampled byte. | Active CC50 checks the raw source byte before lookup and skips destination write on zero. The gate-off C2E0 route fills with the raw sampled byte, including zero. |
| E680 `bit1_set_polygon` (`0xE680..0xEF60`) | Calls DD70 (`0xDD70..0xE680`), which calls C700 (`0xC700..0xC8F0`). | C700 forms `shade = 0x91C488 + (([0xA90A00] & 0xFF) << 8)`. Its source byte indexes `shade`; the resulting shade byte and prior destination byte index the blend table. | C700 tests the raw source byte before shade lookup and skips the write on zero. A zero shade result from a nonzero source is still blended and stored. |
| D420 sprite route to CAB0 (`0xCAB0..0xCC50`) | D420 contains a direct call to CAB0 at RVA `0xD549`; this report does not infer the full producer condition for that call. | No lookup in the observed CAB0 copy loop. | CAB0 tests the raw source byte; zero skips the destination write, otherwise it copies that byte. |
| E680 sprite route to C8F0 (`0xC8F0..0xCAB0`) | E680 contains a direct call to C8F0 at RVA `0xE7BB`; this report does not infer the full producer condition for that call. | `blend = 0x92E3D0 + (([0xA8E6F8] & 3) << 16)`. Lookup index is `(source << 8) | prior_destination`. | C8F0 tests the raw source byte; zero skips the blend and destination write. |

**STATIC_OBSERVED CLUT branch detail.** D420's D555..D595 bytes normalize the row and read the DWORD at `0x96E3E0`. The `gate != 0 && row != 0` branch calls CC50 and returns. CC50's CC71..CC9D setup forms `0x92C390 + (row << 8)`; its D311..D348 loop reads the texture byte, tests that byte, then either skips the pixel or reads the palette byte and writes it. The zero test is on the input texture byte; a nonzero input whose CLUT result is zero still writes zero.

When the gate is zero and row is nonzero, D420 branches to D595, samples into `0xA90A08`, and calls C2E0 at D600. C2E0's C63F..C681 span loads that byte and fills through `rep stosb`/`rep stosd`; there is no source-byte predicate. When normalized row is zero, D420 jumps to D60C instead. Do not merge that row-zero handling with the CLUT or raw-fill description.

**STATIC_OBSERVED control flow; reachability inference for `0x92C490`.** D420 contains a byte load at D5F3 from `0x92C490 + (row << 8) + texel`, but this is not evidence for the active palette base. On normal entry, gate zero reaches D595 with ECX still zero; none of the intervening instructions changes ECX before D5D8 tests it, so D5E2 branches around D5F3. Gate nonzero with a nonzero row returns through CC50; gate nonzero with row zero jumps to D60C. Thus D5F3 is bypassed by the observed normal-entry branches. The directly active base is CC50's `0x92C390` expression.

## Shade and blend indexing

The C700 setup at C805..C841 reads `[0xA90A00]`, masks it to one byte, shifts it by eight, and adds `0x91C488` to form the shade-byte lookup base. It masks `[0xA8E6F8]` to two bits, shifts by 16, and adds `0x92E3D0` to form the blend base. At C882..C89B, nonzero `source` selects `shade[source]`; the prior destination byte then replaces the low index byte, so the blend lookup is `blend[(shade[source] << 8) | old_destination]`.

C8F0 similarly selects the blend base from `[0xA8E6F8] & 3`; its CA51..CA64 loop uses `blend[(source << 8) | old_destination]`. The observed selector-base cases are:

| Selector | Blend base |
|---:|---:|
| 0 | `0x92E3D0` |
| 1 | `0x93E3D0` |
| 2 | `0x94E3D0` |
| 3 | `0x95E3D0` |

These identify table addresses only. No semantic names for the four selector values are inferred. C882's `0x91C488` table is a per-color shade lookup, not the D420 CLUT. The separate 70C0 full-surface remap also references `0x91C488`; it is not part of the C700/C8F0 `0x92E3D0` blend family.

## Destination and address limits

The polygon and sprite bodies read the live surface pointer cell `0xBA0210` and stride cell `0xBA0214`. They compute destination row pointers and clipped pixel pointers (including scratch cells `0xA90944` and `0xA90940`) before the stores. C2E0's fill likewise writes through the clipped destination pointer. This establishes the observed write path as the caller-provided live surface view with a byte stride. It does not establish allocation ownership or prove that any address-derived extent is safe. The pixel helpers' zero tests are not surface-bound checks.

The adjacent sampler evidence gives the source-byte expression `selected_page_base + v*256 + u`, with ordinary base `0x96E6E0 + (((tpage & 0x1F) >> 1) << 16)` and alternate base `0xA6E6E0` when `(tpage & 0x180) == 0x180` (the ordinary page offset is then replaced). This is a static address expression, not proof of a dedicated texture allocation extent.

## Minimal original-byte oracle proposal

The C700/C882 shade+blend slice has already been tested by the separate Unicorn original-byte oracle at `work/renderer_pixel_oracle/c882_report.json`: four selectors, source bytes `0` and `7`, color `0x23`, shade result `0xA6`, and old destination `0x3C`. It runs original bytes from RVA `0xC7F6` to `0xC89F` (SHA-256 `4721786f51d9604adb58e4e62640cc72c6dfbcfc829a31e9d3b08cc66dbb4e5b`). It records that source zero causes no shade-LUT, destination, or blend-table access; source `7` uses `(0xA6 << 8) | 0x3C` and produces `0xD0` through `0xD3`. That is finite evidence for the C882 slice only.

Remaining discriminator status: **PROPOSED_ONLY**. Test one clipped destination byte at a time with synthetic source, old-destination, shade/CLUT/blend tables, and the original entry/helper bytes mapped at their preferred addresses. Run these cases:

1. D420 active CLUT: normalized row 1, gate on; source 0 must preserve old destination, while source 1 must write the byte at `0x92C390 + 1*256 + 1`. Include a palette result of zero for a nonzero source to verify that the resulting zero is stored.
2. D420 gate-off fill: normalized row 1, gate off; source 0 and a nonzero source should both be written by C2E0 as the sampled byte. Keep normalized row 0 as a separate control that reaches D60C.
3. C8F0 blend and CAB0 copy: each gets source zero and one nonzero source. For C8F0 check the same high-byte-source/low-byte-destination ordering for selectors 0..3; for CAB0 check direct byte copy.

For an entry-level run, map complete original functions rather than copying only the quoted instruction windows: D420 plus its called CAB0/CC50/C2E0 bodies; E680 plus DD70/C700/C8F0 as applicable. Seed the original absolute scratch/global state and surface cells from the actual callee contracts, guard mapped writes, and record every modified byte. The quoted slices are discriminators and hash anchors, not a substitute for that setup. The minimum scratch initialization and the entry geometry that reliably yields exactly one pixel remain unresolved; do not claim this oracle has run.

## Reproduction and evidence

Run from the repository root with the bundled Python runtime:

See [README.md](README.md) for the shipped verification commands. Private audit commands are retained only in receipt provenance.

The audit verifies the original image SHA, full closure-span hashes, exact decoded slice boundaries, and the C882 sibling oracle identity/status using the shared baseline context and Capstone 5.0.7. Its output is [`pixel_evidence.json`](evidence/pixels.json); the audit prints the current SHA on each run. The refreshed texture evidence it binds is SHA-256 `958b64439bd4e2c0c402099ef347ca76aab63ee0606ddd7fcc535140a2cd38e4`; closure evidence is `279f4a5c30b1b3eee8bd86f9dbde0096b6da34155e446eb87b35a8c1b87e6c87`; C882's finite oracle report is SHA-256 `07e5bda770330c798c37d07413e505b583ee935fdc254f7b94fd1d1b04644209`. Exact full-function spans and each slice SHA-256 are recorded in the JSON. The prediction and falsifier are in [`pixel_prediction.json`](pixel_prediction.json).

The one-pixel reference kernels are [`hercules_pixels.h`](hercules_pixels.h) and [`privatehercules_pixel.c`](hercules_pixels.c). They use the shared fixed-width packet types and callback-backed bounded table readers. The C882 step validates the selected shade/blend views, returns `NO_WRITE` for source zero before any shade/destination/blend callback, and otherwise reads in the native order: shade, old destination, blend, write. The CC50 step skips source zero; for nonzero input it writes the selected palette byte even when that result is zero, without reading old destination. The C2E0 byte step always writes its input, including zero. These are reference kernels, not complete rasterizers or live callback adapters. VC5 and portable GCC C89 fixtures pass 47 checks each; the runner and hashes are in `pixel_reference_test.json`.

No `recovery.json` state, canonical source, assets, live callback, or gameplay was changed or tested. The A710 upload finite-oracle result in texture evidence is separate from the C882 shade/blend evidence.
