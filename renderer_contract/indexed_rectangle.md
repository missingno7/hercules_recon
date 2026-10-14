# Shared indexed rectangle boundary

`hercules_rectangle.h/.c` extracts the finite D420 -> CAB0 rectangle path from
the existing connected composition fixture into the shared contract target.
The composition fixture and modern indexed reference now call this same utility.
It resolves metadata without reading storage, then renders through the existing
CAB0 zero-skip pixel step. Byte pointers belong to explicit local views; original
addresses remain tokens.

The accepted domain is a complete unblended FT4 (code 0x2c), tpage/clut zero,
red 0x64, unsuppressed staging and the native rectangle lane relationship.
It requires an in-bounds rectangle no larger than 8x8, a 256x256 byte texture
page with pitch 256, and a 320x240 indexed surface with pitch 320. Bounds include
the starting U/V and X/Y when checking the last consumed byte. Unsupported
geometry, shading, blending, clipping and storage layouts are rejected.

`tests/test_original_uploaded_rectangle.py` executes unmodified original A710
upload followed by AC60/D420/CAB0 in the same bounded Unicorn image. Synthetic
detached upload bytes reach the recorded texture sample addresses and surface
stores. External StretchDIBits/GdiFlush endpoints are controlled services;
there is no Windows game process or original device. Code spans are hash-pinned,
instruction/address limits are enforced, and original code remains unchanged.

The base, offset and transparent cases respectively store 3, 6 and 0 pixels.
Each starts with the dirty byte zero. Even the transparent case executes the
request store at RVA 0xCC37, followed by presentation clearing it at 0x6637.
The shared utility therefore reports an accepted presentation request separately
from its pixel-store count. That request does not itself invoke presentation.

[Compact evidence](evidence/uploaded_rectangle.json) links the original cases,
the fresh composition regression under VC5 and GCC x86, and the modern full-buffer
CPU/GPU comparison. The old connected composition event/pixel comparisons remain
equal after extraction. This is finite behavioral evidence; no FUNCTION_MATCH,
general raster equivalence, palette conversion or original-host integration is
claimed. `recovery.json` is unchanged.

The modern consumer additionally tests two consecutive commands with a controlled
upload between callbacks. The first retains its owned sampled tile and the
second sees the new storage generation. Passive frame-entry observations are
not accepted as consumption captures.
