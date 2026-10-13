# Indexed pixel paths

The original 8-bit paths use ordered reads and writes. A GPU implementation
must preserve those byte results and packet order; ordinary RGBA alpha blending
does not implement an arbitrary destination-indexed lookup table.

| Path | Nonzero source | Zero source | Proof |
|---|---|---|---|
| C700/C882 | Shade[source], destination, blend[(shade<<8)\|destination], write | Skip all lookups/writes | Four finite native selector cases |
| CC50 active palette | Write selected palette[source], including a zero result | Skip palette/write | Two finite native cases |
| C2E0 fill | Write sampled byte throughout selected span | Writes zero too | Unaligned nine-byte native fill |
| C8F0 | Destination, selected blend[(source<<8)\|destination], write | Skip destination/LUT/write | Four finite native selectors |
| CAB0 | Write source directly | Skip write | One finite two-byte case |
| D420 normalized row zero | Sample each covered pixel; copy nonzero directly | Skip write | Four gate/texel native triangle cases |

The public one-byte helpers in [hercules_pixels.h](hercules_pixels.h) implement
the first five byte contracts through bounded readers/writers. They do not
implement edge coverage, interpolation or clipping. C fixtures check callback
order, zero handling, invalid views and lookup indices. D420's row-zero byte
effect is the same raw-copy rule as CAB0, but its geometry is a separate path.

D420 normalizes `SAR32(A90A04,6)-0x1e0`; unsigned results greater than 30 become
row zero. A nonzero row and enabled 96E3E0 gate selects CC50; gate zero with a
nonzero row selects the sampled C2E0 fill. Row zero selects D60C irrespective
of the gate. The embedded D5F3 lookup from 92C490 is bypassed by observed normal
entry flow and is not the active CC50 base. See
[polygon_dispatch.md](polygon_dispatch.md) for the rectangle shortcut that
can bypass this row dispatch entirely. E680's shortcut still blends.

Startup LUT generation is not independent allocation: CLUT rows overlap shade
and blend regions. The original startup ramps overwrite 192 blend bytes;
resource-driven updates can replace LUT contents. Use live original bytes or
reproduce writer order, not just startup formulas. See [palette.md](palette.md).

Static original spans and finite execution scopes are recorded separately in
[pixels.json](evidence/pixels.json), [native_pixels.json](evidence/native_pixels.json),
[native_pixel_slices.json](evidence/native_pixel_slices.json) and
[native_polygon_dispatch.json](evidence/native_polygon_dispatch.json).
Original code and data buffers were synthetic/emulated only within the stated
fixtures; no host integration or universal raster equivalence is established.
