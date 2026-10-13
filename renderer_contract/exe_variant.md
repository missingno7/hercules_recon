# HERCULES.EXE static variant comparison

Pinned unpatched SHA-256: `587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d` (275968 bytes).
User-assets SHA-256: `b3ab3ee9e5c973bb245d8eed795e8e7065d7dfc371e412e602b2059af894a551` (275968 bytes).
PE layout and headers identical: **True**; 1 byte difference(s) in 1 contiguous hunk(s).

- File offset `0x94b0` maps to RVA `0xa0b0` in `.text`: pinned `53` → assets `c3`.

Recorded renderer spans checked: 296; renderer/table/DirectDraw-init/known loader-import overlap: **none** (prediction_supported_for_recorded_ranges). The sole hunk is outside all recorded intervals/sites.

This establishes only byte and PE-layout identity at the compared addresses. It does not identify the changed function, prove its behavior, or generalize to other deployed binaries.

Detailed receipt: [exe_variant.json](evidence/exe_variant.json).
