# Indexed blended rectangles

The shared rectangle contract now resolves a bounded FT4 code 0x2e in addition
to its original code 0x2c copy domain. Code 0x2e requires CLUT zero and page-zero
tpage with only selector bits 5..6 set. Staged native lanes and page state must
agree with the packet. Geometry remains an in-bounds unit-scale rectangle no
larger than 8x8, with texture pitch 256 and surface 320x240/pitch 320.

The raw/shaded route discriminator is the red DWORD at A90A00, populated from
the packet's zero-extended red byte. It is distinct from the blend selector at
A8E6F8. Red 0x80 follows E680/C8F0 and blends the raw texture source; other byte
values select DD70/C700 and a shade row before blending. No internal-only red
DWORD 0x180 is accepted as wire input. Finite connected cases measure red 0x80
and 0x7f; the static byte split does not prove every possible packet/content path.

`hercules_rectangle_render_tables` reuses the existing C8F0/C882 one-pixel
helpers. A nonzero source performs a destination-indexed lookup, with a preceding
shade read in the shaded mode. A zero source skips those reads and writes. A
nonzero source whose shade result is zero still blends, and a zero blend result
is written. The legacy render API accepts COPY only; it cannot silently render
a blended plan as raw copying.

The new original CPU fixture joins A710 upload and AC60 dispatch in one pinned
Unicorn image. Explicit synthetic nonidentity shade and nonlinear selected blend
writers precede the original draw. Four selectors are crossed with raw/shaded
routes; separate transparent and shade-to-zero cases distinguish the zero gates.
All cases start with the dirty byte zero and use unmodified original code.

The original raw rectangle sets the request at RVA 0xCA9A; the shaded rectangle
sets it at 0xC8D5. Presentation clears it at 0x6637 in the controlled service
fixture. Both transparent cases request presentation with no shade, blend,
destination read or surface write. These extend the earlier CAB0 request evidence
at 0xCC37 and support the shared request flag independently of pixel stores.

[Compact evidence](evidence/uploaded_blend.json) pins the source, original spans,
writer recipes and per-case receipt/frame hashes. Modern consumers own the
selected 64-KiB blend table and selected 256-byte shade row at consumption time;
all-zero source commands require neither table. This is an explicit bounded
snapshot policy, not proof that original game tables cannot change mid-call.
Unknown observed bytes reject capture. Aliasing writers share one address backing.

These are finite synthetic original-code observations and source comparisons.
No game process, loaded game palette, generic interpolation/clipping, complete
game frame, renderer takeover or FUNCTION_MATCH is established. Historical
recovery state remains unchanged.
