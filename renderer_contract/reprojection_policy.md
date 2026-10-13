# Private resolution reprojection gate

## Prediction and result

The candidate keeps original compatibility values in the existing
`hercules_projection_*_view` types and derives separate double-precision
coordinates from camera XYZ. It never scales the native projected WORDs.
For the original precision collision, camera `(1,0,1000)` and `(2,0,1000)`
both store native `(160,120)` with identity matrix, zero translation, center
`(160,120)`, and focal 256. The scale-6 1920x1440 policy instead returns
X values `120192/125 = 961.536` and `120384/125 = 963.072`.

The private policy applies

```text
effective_focal = sign_extend_16(config.focal & 0xffff)
x = scale * center_x + scale * effective_focal * camera_x / camera_z
y = scale * center_y + scale * effective_focal * camera_y / camera_z
```

The scale-6 output is a deliberate tool-side 4:3 rendering policy. The signed
low WORD focal follows the retained signed numerator after the original
`focal << 16` DWORD wrap, without reproducing the original integer quotient,
product narrowing, or SAR rounding. Center and focal are inputs from the
existing joined tool-side config view, not a reconstructed host struct.

## Input provenance and depth

`hercules_reprojection_from_camera` accepts supplied camera lanes and labels
them `CALLER_SUPPLIED`. This is the path for an actual camera snapshot. It
accepts only sign-extended low-WORD camera values and rejects zero or negative
Z with zeroed private coordinates. A scalar B920 post-call observation has
already clamped camera Z to one, so it cannot independently reveal a previous
nonpositive depth; the function does not infer missing depth.

`hercules_reprojection_eval_single` additionally returns native-compatible
scalar values, but its private camera lanes are derived by the separate B320
reference because scalar B920 capture loses the pre-clamp Z. Their provenance
is `DERIVED_B320_REFERENCE`, not an observed scalar callback. The batch helper
uses B320's unclamped WORD lanes; for Z zero B320 writes the undivided wrapped
`focal << 16` projection result, while the private high-resolution policy
rejects zero depth. A divide fault preserves B320's partial output prefix and
marks later private records unavailable. These helpers do not claim native
memory aliasing beyond the shared source references.

## Gate evidence

`tests/test_original_reprojection.py` performs a fresh run of the pinned
original projection runner, compiles C89 candidate fixtures with VC5 and GCC,
links a C++98 caller, and compares a generated candidate vector source against
all same-run original callback results. The last private replay passed:

- original HERCULES.EXE SHA-256:
  `587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`
- fresh original and candidate receipts are under
  `work/renderer_phase19/reprojection/output/public_candidate_replayN/`
- 14 native cases and 632 original projection assertions
- VC5 and strict GCC candidate runs: 14/14 compatibility plus raw-EAX
  exact; 11 positive private projections and 3 explicit nonpositive-depth
  rejections
- 14 caller-supplied camera cases plus both pointer-free manifest vertices
  pass the private camera-input path
- VC5 and GCC C89 source tests pass; VC5 and GCC C++98 linkage tests pass
- original callback execution used Unicorn 2.1.4; no original Windows process
  was started

The replay report pins source, helper, toolchain, runtime, generated vector,
manifest, and native report hashes. The synthetic manifest binds both
precision-collision identities and its camera/config/screen values to the same
fresh original report. Its private output path must be a fresh directory
under checkout `work/`; the runner rejects original-install/assets overlap
and symlink/junction components.

## Proof boundary

The private formula is not an original viewport equation, pixel renderer,
screen clamp, culling implementation, or GPU path. Input source points are
already signed WORDs, and scalar capture can have irreversibly clamped Z.
Neither wider visibility nor a live scene is established. The current precision
case is synthetic. Real-resource coordinates require a bounded original
producer trace with source-point identity and captured camera/config values;
the shipped resource's file format alone is not proof of runtime consumption.
