# Constant-fill polygons have distinct edge arithmetic

D420 with normalized nonzero CLUT row and disabled color-table gate samples
one source byte, stores it at A90A08, then calls C2E0 to fill the polygon.
Seven native C2E0 fixtures cover nonzero/zero fills, winding, an irregular
quad, shared edges and clipping. Six have paired D60C comparisons. Every fill
case samples once and enters C2E0 once; a zero sample writes 21 zero pixels.

The paths have different coverage in some finite cases. Two shared triangles
cover the same 36-pixel rectangle without overlap, but C2E0 assigns 21/15
pixels to upper/lower triangles while D60C assigns 26/10. In a clipped case,
C2E0 additionally writes `(5,7)`.

The endpoint-ceiling explanation was falsified. Both use plain arithmetic
SAR16, without a preceding 0xFFFF bias. C2E0 uses signed CDQ/IDIV edge setup:
the shared diagonal advances by 0x10000 each row. D60C's reciprocal-derived
step is 0xFFFF, putting later endpoints just below integer x. The clipped
row-7 right endpoints are 0x60004 and 0x5FFFF, accounting for the extra pixel.
Do not replace these two native routes with one common rounding rule.

C2E0 reads F3 top/bottom, not left/right, and separately clamps horizontal
spans to `[0,320)`. These tests do not establish arbitrary coordinate overflow,
every edge division or all palette/UV cases. The raw runner status
BOUNDED_ORIGINAL_BYTES records native execution; the asserted finite fill and
endpoint properties are separate from the falsified cross-route equivalence.

[native_fill_edges.json](evidence/native_fill_edges.json) contains compact
spans, source provenance, sample/fill assertions and endpoint comparisons.
Full traces stay under ignored work. Reproduce with
`tests/test_original_fill_edges.py --exe <original> --receipt work/replay/fill_full.json
--compact-receipt work/replay/fill.json`.
