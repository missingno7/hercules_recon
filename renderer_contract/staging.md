# Effective AC60 staging

[hercules_staging.h](hercules_staging.h) exposes a caller-owned view of the
observed workspace and page cells. It applies the wire decoders to one packet,
retaining fields the original leaves untouched. This view is not a native
memory overlay and never dereferences its raw addresses.

Quad workspace order is packet vertices 0,1,3,2. Polygon UV bytes become DWORD
values shifted left 16; sprite UV bytes remain zero extended. Only the first
red color byte is staged at A90A00. Later GT4 color lanes are not consumed by
AC60. FT3 leaves A909F4 equal to U2 after its observed V2-then-U2 stores.
Sprites retain current color/CLUT and write only dimension WORDs; the view
therefore contains dimension words rather than full native DWORD cells.

BA01FC is a byte. Nonzero values suppress polygon and sprite staging and the
group-60 page refresh. E1, F3, command 01 and traversal continue. The helper
reports `RASTER_SUPPRESSED` for the recognized drawing families. It reports
`IGNORED_RASTER` for 0x68/0x6a, retaining the page refresh when the gate is zero.
The 0x68 producer payload remains opaque. Unknown commands still fail visibly.

For a complete, otherwise-unrecognized group-0x60 header, the helper now
publishes `UNRECOGNIZED_GROUP60` and the gate-dependent page refresh while
returning `APPLY_UNRECOGNIZED`. Callers must inspect that event/state effect;
the status does not claim a recognized packet layout or successful draw.
The [coverage audit](consumer_coverage.md) and [native cases](evidence/native_group60.json)
record the tested tags, header reads and byte-gate conditions.

E1 updates page cells without immediately replacing A909FC. Both E1 and
polygon tpage use `(value & 0x180)==0x180` for the alternate bank, independently
of command bit 1. Ordinary page address is 96E6E0+((selector>>1)<<16);
alternate bank selects A6E6E0. Command 01 returns a deferred move descriptor;
the adapter must perform or preserve that synchronous native operation and
notification before continuing live traversal. The helper does not claim a
VRAM copy succeeded.

The input must contain the bounded decoder's readable minimum even on a
suppressed path. These length checks, recognized-family limits and the local
dispatch-sequence counter are tool behavior, not native AC60 fields. Use the
live callback and link semantics in [notification.md](notification.md) when
building an executing adapter.

The integrated differential compares 171 original workspace/page values,
seven native gate branches and nine page-mask vectors with 74 fresh VC5 C
checks. The gate test places canaries above BA01FC to distinguish byte access.
Rasterizers and A8F0 are stopped at their real call boundaries; this fixture
proves finite staging/dispatch, not pixels or copied bytes. Fixed 8/16 sprites
have native suppressed-branch coverage but no active native staging comparison.
See [native_staging.json](evidence/native_staging.json) and
`tests/test_native_staging.py`.

The corrected source passed a fresh replay of these same 171 fields, seven
gate branches and nine masks. That receipt is preserved separately as
[native_staging_regression.json](evidence/native_staging_regression.json);
the earlier receipt remains a historical source snapshot.
