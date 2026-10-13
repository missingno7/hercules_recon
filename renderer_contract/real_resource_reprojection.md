# Real-resource reprojection candidate

## Prediction and falsifier

The pinned original-resource trace should bind one archived mesh record to its
eight packed point reads, prepared slot-101 inputs, B740/B920 camera and screen
values, same-slot B720 outputs, and three FT4 records. Each FT4 corner should
resolve through its decoded point index to the captured compatibility XY. The
source-built candidate should reproduce all eight native camera/XY pairs and
then calculate a separate scale-six 1920x1440 view from the captured camera
XYZ and the B740 configuration. It must not derive private coordinates by
scaling the stored screen WORDs.

The candidate is falsified by a hash-link or input-hash mismatch, an incorrect
packed-read/source-index join, disagreement among the B6F0/B740/B920/B720
scratch records, a face-index or packet-corner mismatch, an original
compatibility mismatch, or a VC5/GCC failure. The native and private outputs
stay distinct throughout the fixture.

## Finite captured case

The immutable source-v2 input pair for this replay is:

- Manifest SHA-256: `f109016bfe4a059c8d8f06c593bf136c3450cb56bda3fe81c22b0bd85b4d8cf2`
- Native receipt SHA-256: `30cf25c004983e07a6839ef79c24234941f9831a868bf936df9698e266bd61c4`
- HERCULES.EXE SHA-256: `587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`
- ENG1.DLL SHA-256: `01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757`
- HERCULES.FS SHA-256: `affa380e45918448895932cd466ea9a02096b8dcf08644f91b9da5ec34cd48b4`
- MAP_PSX.BIN SHA-256: `78bc5a60ecce1c99e752fb30a16bc5ace4ee80f686365e1da8136a0decbd69cb`

The selected cell is `0x938` at MAP_PSX.BIN file offset `0x10d33c`, selecting
mesh index 156. Original ENG1 executes `2DA90 -> 2DF20 -> 2E600` and emits three
FT4 packets. The eight packed point records occupy file offsets `0x3054` through
`0x3070`. Winding zero maps X to `64-X` before B6F0; Y and Z pass through. The
manifest records each source read, B6F0 input slot, per-point B740 matrix and
translation, B720 output slot, decoded face index, and packet corner. The
producer may read the next source record before B720 returns, so the fixture
joins by the retained preceding B6F0 point index and matching `+8` scratch
slot. It does not join by whichever packed point was read most recently.

The matrix is diagonal 8192 for the eight points and the captured translation
is `(1792,1408,1792)`. Center `(160,120)` and focal 256 come from synthetic
B970/B990 setter inputs; those setters do not run during traversal. The
captured camera Z lanes are all greater than one, so this sample does not probe
the B920 clamp boundary.

| Point identity | Camera XYZ | Native XY WORDs | Private X at 1920x1440 | Private Y at 1920x1440 |
|---|---:|---:|---:|---:|
| MAP_PSX.BIN+0x3054, point 0 | (1792,1344,1920) | (398,299) | 11968/5 = 2393.6 | 8976/5 = 1795.2 |
| MAP_PSX.BIN+0x3058, point 1 | (1920,1344,1920) | (415,299) | 2496 | 8976/5 = 1795.2 |
| MAP_PSX.BIN+0x305c, point 2 | (1920,1344,1792) | (434,311) | 18240/7 | 1872 |
| MAP_PSX.BIN+0x3060, point 3 | (1792,1344,1792) | (415,311) | 2496 | 1872 |
| MAP_PSX.BIN+0x3064, point 4 | (1920,1344,1664) | (455,326) | 35520/13 | 25488/13 |
| MAP_PSX.BIN+0x3068, point 5 | (1792,1344,1664) | (435,326) | 33984/13 | 25488/13 |
| MAP_PSX.BIN+0x306c, point 6 | (1920,1472,1664) | (455,346) | 35520/13 | 27024/13 |
| MAP_PSX.BIN+0x3070, point 7 | (1792,1472,1664) | (435,346) | 33984/13 | 27024/13 |

The three native face index records are `[5,4,7,6]`, `[3,2,5,4]`, and
`[0,1,3,2]`. All 12 native packet corners match the corresponding captured
point identity and XY WORDs. These real-resource values are independent of the
two synthetic camera-X precision-collision cases in
`synthetic_captured_geometry.json`; the mesh trace is not claimed to contain
that collision.

## Replay and result

Generate a fresh resource trace and manifest with the public ENG1 fixture.
The source-emitter comparison and its scope are documented separately in
[eng1_resource_closure.md](eng1_resource_closure.md). All generated geometry
and extracted bytes stay under the fresh work directory.

```powershell
$pythonExe = 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
$meshOutput = 'work/eng1-native-resolution-input'
& $pythonExe -B renderer_contract/fixtures/eng1_real_mesh/run_native_real_map.py `
  --repo-root . --exe D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE `
  --eng1 D:\Prog\hercules_recon\work\discs\pc_install\ENG1.DLL `
  --archive D:\Prog\hercules_recon\assets\pc\HERCULES\HERCULES.FS --output $meshOutput
if ($LASTEXITCODE -ne 0) { throw 'native resource trace failed' }
& $pythonExe -B renderer_contract/fixtures/eng1_real_mesh/make_real_manifest.py `
  --repo-root . --output $meshOutput
if ($LASTEXITCODE -ne 0) { throw 'geometry manifest failed' }
& $pythonExe -B renderer_contract/tests/test_original_reprojection.py `
  --repo-root . --package renderer_contract `
  --exe D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE `
  --output work/native-resolution-reference `
  --geometry-manifest "$meshOutput/real_resource_projection_manifest.json" `
  --geometry-report "$meshOutput/native_real_map_report.json"
if ($LASTEXITCODE -ne 0) { throw 'reprojection comparison failed' }
```

The private candidate replay completed with 14/14 original scalar
compatibility results and raw EAX values exact, alongside the existing 11
positive-depth and 3 explicit depth rejection cases. The separate real-mesh C
fixture passed with VC5 and strict GCC: 8/8 source-built compatibility camera
and XY pairs, 8/8 caller-supplied private projections, 3 faces, and 12/12
packet-corner XY checks. The receipt is
`work/renderer_phase19/reprojection/output/public_real_resource_replay8/report.json`
(SHA-256 `5c92a6d6d744963ba901e4f5c67e58f77533ef271711d9b693367e359660923e`);
its fresh scalar-oracle report has SHA-256
`7a3dc5c6182486b347cf1643e5e87003352ce84af4f1e8aee17b15c702f3ffd3`.
Its candidate source hashes include the private header's explicit
non-overlapping input/output storage precondition.

## Limits

This proves one bounded resource-backed producer call in a controlled camera
context, not a shipped live scene or general traversal contract. The separate source emitter comparison passes three cases and 481 checks;
upstream preparation and traversal C equivalence remain unverified. The utility's floating-point formula
is a private tool-side display policy, not an original viewport equation or a
pixel renderer. Inputs are sign-extended 16-bit camera lanes; captured B720
screen values are already WORD-quantized, and scalar camera Z has already been
clamped. The sample does not establish wider visibility, clipping, all
winding/culling behavior, other asset routes, actor identity, or GPU behavior.
