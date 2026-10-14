# Actual archive resource and finite sprite contract

The public `test_original_title_screen.py` fixture executes unchanged A640/A710
and AC60/CAB0 with actual archived MENUB pixels and a controlled two-sprite chain.
The full visible output equals the source's top 240 rows. `test_original_cif_palette.py`
executes host slot 145 (5760), 68C0, 5210, original CRT buffering and 6780 against
the separate MAINMENU CIF. Only external file reads/seeks and single-thread locks
are modeled; execution stops before 69A0's presentation body.

The CIF contains 256 RGB triplets at 0, four blend banks at 0x300, one shade table
at 0x40300 and 31 CLUT rows at 0x50300, consuming 0x52200 bytes. The original
reads 256 triplets then those three spans, writing blend/shade/CLUT in that order.
Logical palette entries are RGB with flags 4; DIB entries are BGR with padding 0.
Endpoint forcing is visible in the pinned original helper; this real palette's
endpoint inputs already equal black and white. The shared CIF view borrows these
spans and does not infer a native gate from a resource alone.

The original palette fixture seeds prior 71A0 outputs from independently verified
formulas and hashes. It does not execute 71A0. CLUT row32 retains its first 64
startup bytes and aliases the blend prefix for the final 192 bytes. With this
resource and startup state, the original first gate mismatch is row32 byte64,
and the final gate is 1. This is a resource/state-specific result.

The sprite resolver uses the existing geometry and CAB0/C8F0 pixel steps. Its
supplied target represents native surface availability. It requires the measured
bottom bound 240, ordinary aligned 256x256 pages, and post-clip UV spans contained
in one page. Suppressed or empty sprites produce no draw; accepted zero-source
draws preserve destination bytes and still request presentation. Shared tests
cover raw selectors, fixed sizes, clipping, missing storage and unsupported bounds.

No FUNCTION_MATCH is promoted. Actual native resource consumers, finite controlled
packet equality and modern opaque RGB presentation do not establish TITLE's
producer, original timing, gameplay, source identity or a complete renderer.
Compact provenance and current acceptance are in [title_resource.json](evidence/title_resource.json).
