# D420 normalized-row-zero and rectangle dispatch

## Finding

For original `HERCULES.EXE` (`587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`), D420 normalizes the encoded row as `SAR32([0xA90A04], 6) - 0x1e0` and maps unsigned results above `0x1e` to row zero. Row zero reaches D60C whether the gate at `0x96E3E0` is clear or set. It bypasses both the active CC50 palette route and the clear-gate/nonzero-row C2E0 fill route.

The D60C..DD70 body samples one byte from the selected texture at `base + (v & 0xff)*256 + (u & 0xff)`. DC8A loads the byte, DC92 tests it, DC94 skips a zero sample, and DC96 stores nonzero samples directly to the clipped surface row. The body contains no palette, shade, blend, or color lookup. In four finite CPU fixtures, changing the row gate did not change this route; zero texels left destination bytes unchanged, while nonzero `0x5a` samples wrote `0x5a` to the four pixels covered by the synthetic 2x2 right triangle. The configured shade-LUT decoy was not read.

The original D420 entry also has an axis-aligned 4-vertex fast path at D549 that calls CAB0 and returns before D555 row dispatch. A finite native case reached that path and copied nonzero texture bytes directly, skipping the zero byte and not reading the shade-LUT decoy. A separate E680 dispatcher has a similar rectangle predicate but calls C8F0; its finite case read destination bytes and used the selector-2 blend entries, so this route retains blending. Its source-zero sample was skipped.

## Exact static evidence

| Original span | RVA range, end exclusive | Size | SHA-256 |
| --- | ---: | ---: | --- |
| D420 full entry | `0xD420..0xDD70` | 2384 | `4645c60cf0a357ed4aae074c3487a2755384852d0874e2f12b352b994544e07e` |
| D420 row normalization and gate dispatch | `0xD555..0xD600` | 171 | `5d49addf325ea9fe649761a07c6c1407543a2896d2720c7e5f4ad11b4de22e1f` |
| Row-zero body | `0xD60C..0xDD70` | 1892 | `2ed9dc4341703ee82f789528b427f6333e6d916bec11148fe2cfecff14119b4a` |
| Row-zero source test/store loop | `0xDC28..0xDCA1` | 121 | `22edfb81206a1643675b5c9f2f49f1c9b9cc677bda3064a4d6b4f14c43e8f777` |
| D420 rectangle predicate/call | `0xD420..0xD555` | 309 | `dcc20dba6d4c0e286228320b59ee3c96405741f03e24a262f573523cc1a4f9da` |
| E680 mode/shape dispatcher | `0xE680..0xE7C7` | 327 | `1ac531aa802148e641444cb62451c9903fe6e59b046329d0ea0a80f463ccc75a` |
| CAB0 entry through return used by the finite D420 case | `0xCAB0..0xCC45` | 405 | `5ca22ca070b69abaf98a0da5c794b0642cf816026b7ff87c794d6a82f6d9c71f` |
| C8F0 entry through return used by the finite E680 case | `0xC8F0..0xCAA8` | 440 | `74bfb760505e5695cf5f1668e390d07046a32f7014c506706a85f9488870d52c` |

The D420 CAB0 predicate requires nonzero `[0xBA01F0]` and `[0xBA0210]`, count 4, the signed screen-axis and corner equalities, matching UV-axis equalities, and signed 16.16 unit-scale equations. A match stores the low 16-bit screen deltas, converts U0/V0 to integer origins, calls CAB0, and returns. Failed predicates continue into D555. E680 adds mode `[0xA90A00] == 0x80`, uses the same geometry/UV shape checks, and calls C8F0 on a match; the generic failure path continues at E7C7.

The complete static span list and decoded predicates are in [polygon_dispatch.json](evidence/polygon_dispatch.json). These are `STATIC_OBSERVED` claims from pinned original bytes and Capstone 5.0.7, not execution results.

## Bounded original-byte fixtures

[native_polygon_dispatch.json](evidence/native_polygon_dispatch.json) records six successful Unicorn 2.1.4 x86-32 cases executed over unchanged original instructions with synthetic geometry, texture, table, and surface data. It records the original image hash, dependency hashes, execution windows, original-code immutability checks, data restoration, read/write traces, and per-case before/after surface hashes.

Four cases cover row zero with the gate clear/set crossed with source texel zero/nonzero. Each uses a three-vertex triangle at `(1,1),(3,1),(1,3)` and constant `U=V=0`; each traces through D555 and D60C to DC8A. The zero cases produce no surface writes. The nonzero cases write `0x5a` at surface offsets `9,10,17,18`. Neither case reads the seeded shade-LUT decoy.

The fifth case enters D420 with a 2x2 axis-aligned quad, traces D549 â†’ CAB0 â†’ CBD6, and bypasses D555/D60C. It writes source values `0x5a,0x77,0x6b` at surface offsets `10,17,18`; the zero source at offset 9 is skipped. The sixth enters E680 with mode `0x80` and selector 2, traces E7BB â†’ C8F0 â†’ CA51, reads the destination at each nonzero source pixel, and writes seeded blend results `0xe1,0xe2,0xe3` at offsets `10,17,18`. All cases record unchanged original code and restored original `.data` after each run.

These fixtures establish only the listed finite cases. They do not establish arbitrary polygon edges, clipping, UV gradients, general quad behavior, whole-function equivalence, a full software renderer, GPU behavior, gameplay, or a `FUNCTION_MATCH`.

The raw-bit predicates in [hercules_quad_dispatch.h](hercules_quad_dispatch.h)
implement the static shape checks only. They do not implement interpolation,
span edges or pixel loops. `tests/test_quad_route.c` tests nine discriminators.
Run `tests/test_original_polygon_dispatch.py` for the six native cases.
See [verification.md](verification.md) for explicit image and receipt arguments.
