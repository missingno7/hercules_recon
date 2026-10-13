# Skewed convex FT4 original-byte fixture

The bounded runner is `renderer_contract/tests/test_original_skewed_quads.py`. It imports the public quad interpolation helper, which imports the public polygon-edge helper. Five signed-16 XY / unsigned-byte UV fixtures and the prediction/falsifier are embedded in the runner.

Run from the renderer repository root with explicit original EXE and receipt paths:

```powershell
& 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' -B renderer_contract/tests/test_original_skewed_quads.py --exe ..\discs\pc_install\HERCULES.EXE --original-root . --receipt work/replay/skewed/full.json
```

The runner writes `compact_evidence.json` beside the full report. Both output paths are guarded to remain under ignored `work/`. It verifies the pinned EXE and Unicorn runtime, records SHA-256 identities for the EXE, runner, public helpers, runtime DLL, and original code spans, and checks that original code bytes remain unchanged.

The result is scoped to five compact convex quads across D60C row-zero, CC50 palette, and E680 mode-0x80 selector-2 routes. It checks Q30 edge/span coverage and UV reads, top/bottom clipping, and a pair of quads sharing a slanted boundary. It does not claim equivalence outside those bounded inputs and routes.

The [published receipt](evidence/native_skewed_quads.json) records 15 original
route runs: 143 texture reads and 143 output writes per route across the five
shapes. All match the independent Q30 model. The shared-edge pair has no overlap
or gap against that model. Exact rational edge coverage disagrees at `(8,4)`
in the right-hand fixture: original Q16 right x is `0x8ffff`, so the span ends
at 8 and omits that pixel. This finite counterexample constrains a faithful
backend; rational edge interpolation is not interchangeable with the original
reciprocal arithmetic.
