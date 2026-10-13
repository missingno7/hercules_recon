# Cross-module PC packet evidence

This receipt extends the existing bounded packet decoder only where original
ENG3 and TITLE bytes add reachable producer paths or clarify command `0x68`.
It reuses the established fixed-width raw-address header and does not define a
PSX aggregate, a GPU API, or a common frame struct. Original image identities,
bounded native spans, dispatch bytes, and cited evidence hashes are in
[module_packets.json](evidence/module_packets.json); the exact original spans are checked by `tests/validate_originals.py`.

Prediction before the final audit pass: ENG3's table-index-4 route would write
linked `0x68` records, and AC60 would treat that tag as a recognized command
whose dispatch path skips rasterization after the shared group-`0x60` page-base
refresh. This would be falsified if the original emitter wrote another tag or
if the AC60 branch reached either raster call, read a payload field, or lacked
the page-state preamble.

## Producer and consumer matrix

| Original producer route | Host packet constructor | Command bytes | Producer stride | Evidence level |
|---|---|---|---:|---|
| ENG1 mesh producer `0x2df20` | slot 119 â†’ HERC `0xb660` | GT4 `0x3c/0x3e` | `0x34` | Reused `STATIC_OBSERVED` evidence in `evidence/geometry.json`; direct slot-117 FT4 call is absent from this span. |
| ENG1 tile and command writers | Native packet writes | variable sprite `0x64` | `0x14` | Reused existing writer and finite differential receipts in `evidence/packets.json`. |
| ENG3 `0x204c0`, called from `0x20190` | slot 119 â†’ `0xb660`; slot 117 â†’ `0xb5c0` | GT4 `0x3c/0x3e`; FT4 `0x2c/0x2e` | `0x34`; `0x28` | `STATIC_OBSERVED`. `0x20190` has native direct-call sites `0x1a89d`, `0x1ade8`, `0x1ae0d`. |
| ENG3 `0x231a0`, called at `0x901d` | slot 117 â†’ `0xb5c0` | FT4 `0x2e` | `0x28` | `STATIC_OBSERVED`. |
| ENG3 handler table index 5 â†’ `0x23440` | slot 117 â†’ `0xb5c0` | FT4 `0x2c/0x2e`, supplied by helper `0x23b00` | `0x28` | `STATIC_OBSERVED`; dispatch calls at `0x8c68` and `0x8fed` index the six-entry table at `0x35010` using object byte `+0x23`. |
| ENG3 handler table index 4 â†’ `0x22f00` | No XY constructor | command byte `0x68` | `0x0c` | `STATIC_OBSERVED`; writes a link at `+0`, a DWORD at `+4` then overwrites byte `+7` with `0x68`, a DWORD at `+8`, and advances by `0x0c`. |
| TITLE `0x17f50` and `0x181e0` | slot 117 via wrapper `0xc820` â†’ HERC `0xb5c0` | FT4 `0x2e`; FT4 `0x2c/0x2e` | `0x28` | `STATIC_OBSERVED`; both paths appear in the title-mode table or its direct caller route. |
| TITLE `0x18bf0`, called from `0x18a90` and `0x18b20` | slot 118 via wrapper `0xc830` â†’ HERC `0xb570` | FT4 command/color DWORD supplied by helper `0x189a0` | `0x28` | `STATIC_OBSERVED`; the `0xb570` body is byte-identical to slot 117's `0xb5c0` body. |
| TITLE `0x186e0`, mode-table entry 7 | No host XY constructor in this writer | It writes FT4-shaped coordinates and copies a computed DWORD to `+4`; this receipt does not classify that DWORD's command byte. | `0x28` | `STATIC_OBSERVED` within the bounded title producer set. |

ENG3's actual polygon routes establish FT4 and GT4 command families; no FT3
producer is established by this cross-module pass. The previously audited
variable sprite route is separate. The TITLE mode table at RVA `0x220d0`
contains `0x181e0` and `0x186e0`, and `0x2b70` dispatches through that table.
The independent TITLE finite producer receipt remains
`evidence/packets.json#title_ft4`; it is producer behavior only,
not packet-decoder, AC60, or pixel equivalence.

## Command `0x68`

The later [ENG3 handler trace](eng3_handlers.md) follows the remaining indices
0-3: indices 0/1 reach FT4 writers, while 2/3 prepare state and delegate to the
same indexed renderer. The [TITLE handler trace](title_handlers.md) follows
mode indices 2/3/4/6: child dispatch, a linked 12-byte `0x68` record, and FT4
writers. TITLE's `0x189a0` helper proves command-word high bytes `0x2c/0x2e`
on its reviewed paths. These finite closures add no packet family and do not
exclude unreviewed or computed producer paths.

AC60 reads the common link DWORD at packet `+0` and command byte `+7`. On the
group-`0x60` route, it first checks `BA01FC`. When that gate is zero, it writes
the selected page base to `A909FC` from page state at `A8E6F4`, and may replace
that value from `A8E6FC`. It then clears command bit 1 and compares only against
`0x7c`, `0x74`, and `0x64`. Tag `0x68` misses all three comparisons; branch
`B119` returns to the traversal continuation before calls `C8F0` (`B162`) and
`CAB0` (`B16C`). The page-base change is a dispatch-side effect, so this path is
not a pure no-op. If `BA01FC` is nonzero, AC60 returns to traversal before the
page-base write and command comparisons.

The decoder therefore reports tag `0x68` as kind `OBSERVED_UNDRAWN` and status `KNOWN_UNDRAWN`, with an
8-byte consumer minimum and no payload fields. That minimum covers only the
shared link and the tag. ENG3's producer stride is 12 bytes, which is not
promoted to the consumer minimum or a claimed universal allocation size. The
producer-populated DWORD at `+8` remains opaque. The 4-byte-aligned stride and
bytes through `+0x0b` are static producer observations only.

Clearing bit 1 makes `0x6a` take the same consumer branch. No audited first-party
producer writes `0x6a`, so the decoded metadata sets
`producer_tag_observed` only for exact tag `0x68`. Other unverified tags remain
`UNKNOWN` / `UNRECOGNIZED`; in particular, this does not bless the broader
group-`0x60` range.

The public decoder in [hercules_packets.h](hercules_packets.h) has six
additive synthetic checks (43 total packet checks). It deliberately consumes
only an 8-byte header for 0x68/0x6a and invents no payload meaning. The AC60
staging fixture separately observes 0x68's page refresh before traversal.
No first-party 0x6a producer or universal command coverage is claimed.
