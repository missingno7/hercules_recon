# Generic four-vertex interpolation

The native quad is an ordered four-edge polygon. A tested generic quad does
not sample like either affine triangle split. Do not assume a GPU's triangle
decomposition reproduces the host's interpolation.

The fixture uses native circular workspace XY `(1,1),(5,1),(5,5),(1,5)` and
UV `(0,0),(4,0),(7,7),(0,4)`. The corresponding packet order is 0,1,3,2,
as described in [staging.md](staging.md). These are wire-representable unsigned
UV bytes, staged in the fixture as Q16 DWORDs. Their irregular corners fail
the native unit-scale rectangle shortcuts.

D420 row-zero, CC50 palette, and E680 neutral-red/selector-2 generic routes
each cover the same 4×4 area and make the same sixteen UV reads. All reads
match the finite edge/span model. Both affine triangle splits match only
fourteen of sixteen samples: at `(3,3)`, native UV `(2,2)` differs from the
0–2 split's `(3,3)`; at `(4,3)`, native `(4,3)` differs from the 1–3 split's
`(3,2)`.

A normalized bilinear model also matches this particular axis-aligned screen
rectangle. That does not establish general bilinear interpolation for skewed
quads or distinguish it from native edge/span progression. The predicted
earlier scanline discriminator was falsified and is retained in provenance.

Finite route/model comparisons, source reads and hashes are recorded in
[native_quad_interpolation.json](evidence/native_quad_interpolation.json).
Run `tests/test_original_quad_interpolation.py` with the common explicit
image/receipt arguments. No arbitrary quad domain, clipping, perspective
correction, GPU result or full-function equivalence is claimed.

The [packed-UV counterexample](packed_uv_counterexample.md) subsequently
falsified separate-component Q16 pixel accumulation on a different skewed
fixture. The original carries between packed lanes; the model predicts U=150
where the original reads 151. The sixteen matching samples above and the
[fifteen skewed-quad runs](skewed_quads.md) retain their finite scope. They do
not establish a general scan iterator. Source bytes also enter the captured
V accumulator; their possible effect on later emitted addresses is unresolved.
