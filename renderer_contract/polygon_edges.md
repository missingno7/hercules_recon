# Bounded polygon coverage and interpolation

Twenty-two unchanged-original D420 runs cover eleven small triangles through
D60C row-zero and CC50 palette routes. Selected coverage and UV reads match a
finite integer edge/span model, and the two routes agree for each tested shape.
This does not establish E680's separate shaded/blended geometry or arbitrary
coordinate and overflow domains.

Native initialization at EXE RVA 72A0..72CC builds 2,048 reciprocals:
R[0]=0 and R[i]=floor(2^30/i). The fixture executes that initializer before
drawing, then stops before external window setup. These are reciprocals of
edge/span lengths, not per-vertex reciprocal depth.
The original indexes this table with the low 11 bits of its difference/span
value. The fixture model divides by the full positive length only because its
tested lengths are small. It is not an arbitrary-width native implementation;
zero-index and nearby 0x800 guards need separate treatment beyond these cases.

The selected shapes use integer scanlines `[top,bottom)`. Signed Q16 x
endpoints are floored by SAR16, with pixels in `[floor(left),floor(right))`
clamped to `[0,320)`. A pair of shared-edge triangles fills a 4×4 rectangle
exactly once. Reversed winding matches in the finite case. F3 top A909B4 and
bottom 43A48C affect these paths; F3 left A909B0 and right 43A488 are not read.

Edge and span progression multiply signed deltas by the reciprocal entries
and reduce with SHRD30. Horizontal UV stepping uses the floored span width,
starting from the left-edge attribute at floor(left_x). Exact continuous
interpolation cannot replace those intermediate quantization steps without
checking the resulting samples.

The wire-valid discriminator has vertices `(-2,-2),(5,1),(1,6)`, UV bytes
`(0,2),(2,2),(1,0)`, and F3 top=1,bottom=5. At pixel `(1,2)`, original D60C
and CC50 read UV `(1,1)`, offset 0x101, texel 9. Exact rational barycentric
interpolation at that integer point floors to `(0,1)`. The finite reciprocal
model matches `(1,1)`. This is a confirmed counterexample to that mathematical
model, not a defect in the original or a claim about an implemented GPU backend.
The earlier negative-V fixture is explicitly an internal-state test; it is not
an AC60 wire-input claim. Changing top from 1 to 0 preserves fourteen common
UV samples per route in the paired discriminator.

[native_polygon_edges.json](evidence/native_polygon_edges.json) records compact
case hashes/counts, exact counterexamples, model hashes and original spans.
Reproduce with `tests/test_original_polygon_edges.py` and the common explicit
EXE/receipt arguments in [verification.md](verification.md). Full traces remain
in ignored work. No complete rasterizer or new native ABI is implemented.
