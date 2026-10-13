# Original D60C packed UV counterexample

The separate-component Q16 UV model has a finite counterexample on one
strictly convex FT4 workspace fixture. At `(318,2)`, the public model predicts
`(150,39)` and the pinned original D60C route reads `(151,39)`.

The preceding pixel `(317,2)` shows why. The original loop has `EBX=0x5555f9b5`
and `EDX=0xaaaafde7`; `ADD EBX,EDX` wraps to `0x0000f79c` with carry set.
Then `ADC CL,DL` evaluates `0xaf + 0xe7 + 1`, yielding `0x97` (151) for the
next U byte. The test independently checks both the 32-bit result/carry and
the byte-lane result against captured original registers.

The same fixture runs with every texture byte set to 1 and then 255. Each run
has 36 texture reads and 36 surface writes, and the complete ordered pixel/UV
streams are equal. The sampled texel changes AL and the full EAX accumulator
before the V update. This receipt records that register feedback and the
equal finite streams; it does not claim universal texel independence or settle
whether that feedback can change a later carry into CH on another span/state.

The acceptance status is `FINITE_ORIGINAL_COUNTEREXAMPLE`. It confirms only
the selected public-model mismatch and its captured packed-carry arithmetic.
The fixture preloads the native-order workspace and uses synthetic texture and
surface memory. Packet staging, AC60 traversal, shaded routes, actual host
surface addressing, and arbitrary-frame equivalence are not covered.

The [published receipt](evidence/packed_uv_counterexample.json) records the
selected registers, model mismatch and finite checks. Reproduce from this
checkout with explicit read-only original input:

```powershell
python renderer_contract/tests/test_original_packed_uv.py --original-root . --exe D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE --receipt work/replay/packed_uv.json
```

The proposed separate-component iterator remains an unaccepted scratch
candidate. This counterexample does not invalidate the earlier finite
triangle/quad fixtures or the captured-delta edge arithmetic utility; it
limits their use as a complete pixel-advance model.
