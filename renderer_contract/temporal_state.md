# Temporal state feasibility evidence

**Result:** static layout and writer evidence supports limited camera X/Y history, but there is no basis to enable interpolation. A bounded Unicorn test also observed the original camera-history routine across two controlled invocations. Those invocations are not full simulation updates. The actor-pool address can be recycled, and the inspected mesh selector has no established association with an actor-pool object.

The compact inventory is [`evidence/temporal_state.json`](evidence/temporal_state.json). It keeps `STATIC_OBSERVED` layout findings separate from the `BOUNDED_EQUIVALENT` controlled original-code calls. The controlled-case prediction is in [`fixtures/temporal/prediction.md`](fixtures/temporal/prediction.md); it was recorded before that native run.

| Area | Static observation | Feasibility limit |
|---|---|---|
| Camera pose | ENG1 current X/Y/Z are DWORD cells at `0x70ca0/0x70ca4/0x70ca8`; current angle X/Y are DWORD cells at `0x70cd0/0x70cd4`. At entry, `0x2fb70` copies current Y to prior Y `0x70cc8`, then current X to prior X `0x70cc4`. | Only X/Y have measured prior cells. The controlled calls confirm this writer sequence but do not sample two complete simulation updates, camera-follow motion, or downstream use. |
| Camera/render matrix | `0x2f840` writes seven DWORDs of the 32-byte transform at `0x70c60`; `0x2da90` copies it into interface transform scratch. | This is shared camera/render state. No separate object matrix or retained current/previous matrix is established. |
| Actor candidate | The normal pool owner cell is `0x72860`, with `0x134` stride. The measured view has DWORD XYZ at `+0/+4/+8` and WORD yaw/pitch/roll at `+0x70/+0x72/+0x74`. Native traversals at `0x256e0` and `0x258e0` copy some current XYZ DWORDs to `+0x9c/+0xa0/+0xa4` before callbacks, subject to flags. | Allocation selects `kind WORD == 0` slots. No per-slot generation or serial is established, so an equal raw address is not stable identity. The snapshot is a candidate history writer, not a proven interpolation contract. |
| Mesh and animation | `0x2df20` uses a WORD selector within the current mesh/header table. The actor view has a frame-array pointer at `+0x110` and WORD frame index at `+0x114`. | No selector-to-actor association or frame/vertex backing-store lifetime is established; projected-vertex scratch is reused. |
| Frame boundary | ENG1 resets OT state at `0x5600`, submits `current+0x88` at `0x20616`, then rotates the frame view and increments context DWORD `+0x38` before `0x60f9`. | The increment is a render-iteration tick; it does not identify simulation updates or objects. |

The controlled test invokes original ENG1 `0x2fb70` at its true entry with the bounds-helper flag clear and the follow-iteration byte `0x70cfd` set to zero. It executes 32 instructions per call, skips the actor-follow loop and bounds helper, and returns inside the pinned body. On each call it writes prior Y, prior X, then derived grid Y/X/Z DWORDs. With distinct seeds, the second call overwrites prior X/Y with its own current X/Y. Current XYZ remain unchanged. This establishes the camera history writer path under controlled state only.

Possible history breaks include camera seeding/reposition, restart/setup, scene/header replacement, pool reset, and actor spawn/despawn/reuse. These are conservative static boundaries; no explicit camera-cut or scene-generation field is established. The source-built host's missing routine `0x28290` is unrelated and is not a renderer-readiness blocker.

## Reproduction

Run both commands from PowerShell after the package files are installed in the repository. All paths, including output paths, are explicit. The scripts require outputs to be both lexically and physically below the repository's `work/`; they reject redirects, outputs overlapping originals or their directories, and paths under assets.

```powershell
$repo = (Resolve-Path .).Path
$python = 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
$originals = 'D:\Prog\hercules_recon\work\discs\pc_install'
$exe = Join-Path $originals 'HERCULES.EXE'
$eng1 = Join-Path $originals 'ENG1.DLL'
$out = Join-Path $repo 'work\temporal_state'
& $python -B (Join-Path $repo 'renderer_contract\tests\audit_temporal_state.py') --repo-root $repo --exe $exe --eng1 $eng1 --output (Join-Path $out 'audit.json')
if ($LASTEXITCODE -ne 0) { throw 'Static temporal audit failed' }
& $python -B (Join-Path $repo 'renderer_contract\tests\test_original_camera_history.py') --repo-root $repo --exe $exe --eng1 $eng1 --output (Join-Path $out 'camera_history.json')
if ($LASTEXITCODE -ne 0) { throw 'Bounded original camera-history test failed' }
```

The scripts use the existing `scripts/pe.py`, `renderer_contract/tests/oracle_common.py`, Python 3.12, and pinned Unicorn 2.1.4 from `C:\tools`. They read the hash-pinned images, do not patch original bytes, and write receipts only below `work/`.

## Confidence limits

The evidence does not establish two full simulation updates, stable object identity, mesh/object association, object-matrix separation, animation-frame lifetime, camera-cut detection, or interpolation readiness. It describes static original bytes and one bounded equivalent path through the original camera-history routine; it makes no gameplay, renderer, presentation, or source-equivalence claim.
