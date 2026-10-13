# Shaded and raw blended polygons

E680 compares the full DWORD at A90A00 to 0x80. Ordinary AC60 polygon input
supplies only a zero-extended red byte. Wire red 0x80 bypasses shading and
blends raw source indices, even if shade row 128 contains nonidentity data.
Other wire red values enter DD70 and select that shade row before blending.
The rectangular shortcuts select C8F0 and C700 respectively; generic paths
select E7C7 and DD70's edge body.

Seventeen original-code cases cover five positive shapes across wire 0x7F,
wire 0x80 and internal-only DWORD 0x180, plus two zero-source triangles.
The 0x180 discriminator enters DD70 and selects low-byte shade row 128; it is
not reachable from an ordinary AC60 packet and is not a gameplay-mode claim.
Nonidentity shade rows and distinct blend entries distinguish all positive
outputs from the opposite path. The zero cases still sample ten texels each
but perform no shade, blend or destination accesses. The observed zero tests
are E581 (DD70) and EE82 (raw E680).

Selected coverage and UV addresses match the small reciprocal/Q16 model used
by [polygon_edges.md](polygon_edges.md). Both bodies read F3 top and bottom;
neither reads F3 left/right. Shared triangles fill the selected 4×4 rectangle
once. Horizontal bounds, negative XY and rectangle shortcuts are covered.
Native reciprocal indices are masked with 0x7FF; the model's full-length
division is limited to its small finite cases. No overflow/general-domain
equivalence or arbitrary quad decomposition is proved.

The runner's execution status is BOUNDED_OBSERVED; its project proof field is
BOUNDED_EQUIVALENT for the explicit model/read/write assertions only.
[native_shaded_edges.json](evidence/native_shaded_edges.json) records compact
case digests, original spans, mode domains and source provenance.
Reproduce with `tests/test_original_shaded_edges.py` using the common arguments
in [verification.md](verification.md). No Windows/GPU/gameplay execution occurs.
