# ENG1 mesh points to packet corners

This package records one bounded association through original PC x86 code. Synthetic point and face records are emitted by ENG1's native mesh path, transformed by the original HERCULES callbacks, and copied into original GT4 or FT4 packets. It does not add a renderer implementation, install a callback, or establish live scene identity.

The test entry point is [test_original_eng1_association.py](fixtures/eng1_association/test_original_eng1_association.py); it imports the adjacent shipped [fixture runner](fixtures/eng1_association/run_fixture.py) and [pre-experiment prediction](fixtures/eng1_association/prediction.md). The tool-side [observer record header](eng1_geometry_association_observer.h) defines raw snapshots, not an engine callback ABI. Each successful replay writes its machine trace and compact receipt under the explicitly selected output directory in `work/`; it never writes evidence during replay into `renderer_contract/`.

## Reproduction

Run from the repository root with all paths explicit:

```powershell
& 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' `
  renderer_contract/fixtures/eng1_association/test_original_eng1_association.py `
  --repo-root 'D:\Prog\hercules_recon\work\renderer_interface_repo' `
  --originals 'D:\Prog\hercules_recon\work\discs\pc_install' `
  --projection-report 'D:\Prog\hercules_recon\work\renderer_interface_repo\work\renderer_phase14\public_projection1\report.json' `
  --output 'D:\Prog\hercules_recon\work\renderer_interface_repo\work\renderer_phase14\eng1_association\public_replay_2'
```

The test requires explicit repo, original install, current public projection report, and a new unused output directory strictly inside this checkout's `work/`. It rejects reparse-point path components and checks the four original PE hashes. It validates every source hash and tool/artifact hash recorded in the supplied projection report, derives the projection objects from its successful compile commands, then relinks those public objects at `0x11000000`. This avoids the base collision between the projection DLL at `0x10000000` and ENG1's preferred base. No private runner, prediction, projection source, or private projection object is read at runtime. Use a new output path for each replay; the machine report and compact `eng1_geometry_association.json` receipt are both written there for review and later integration.

The pinned public projection run reports 14 cases and 632 assertions. The association test does not rerun those 14 cases: it checks the pinned report and artifacts, relinks the already-built public objects for the producer fixture, and then runs the four finite association cases. Missing or stale inputs fail before execution.

## What the fixture proves

The runner maps HERCULES.EXE at `0x00400000` and ENG1.DLL at `0x10000000`, independently at their pinned bases. It copies the complete 250-DWORD host transport (1,000 bytes, source HERCULES.EXE RVA `0x361B8`, SHA-256 `52654fb050caa552b03c692bd3a1da0e700675bb50605394c8afb92595a039f8`) to ENG1 `0x10070F20` before setting fixture state. Original table word 4 is `0x00BA25C0`, so the observed point-stage base is `word4+0x40 = 0x00BA2600`, with 8-byte stage records. In the observer header, `POINT_STAGE_OFFSET`
is relative to borrowed transport word 4; the captured EDI already points
to the first stage record and must not have that offset added again. Word 101 is the B6F0 input callback; word 106 is B920. In the bridge comparison, only word 106 changes.

The primary fixture uses four packed source points at `0x0C020104 + 4*i`:

| Index | Synthetic XYZ bytes | Source address | Stage address | Camera XYZ | Projected XY |
|---:|---:|---:|---:|---:|---:|
| 0 | `(31,20,64)` | `0x0C020104` | `0x00BA2600` | `(31,20,320)` | `(163,121)` |
| 1 | `(-30,0,64)` | `0x0C020108` | `0x00BA2608` | `(-30,0,320)` | `(157,120)` |
| 2 | `(0,80,64)` | `0x0C02010C` | `0x00BA2610` | `(0,80,320)` | `(160,127)` |
| 3 | `(34,12,96)` | `0x0C020110` | `0x00BA2618` | `(34,12,352)` | `(163,121)` |

The input is explicit synthetic data. The shared projected coordinate for points 0 and 3 distinguishes their identities by source address and camera Z. The trace records original source-read PCs, B6F0 input pointer/XYZ, B920 camera/projected results, and B720's output pointer. In the GT4 cases, projected XY is written back to the same 8-byte stage record address used for that point. The FT4 tail captures B720's output pointer separately; its observed output address trails the current B6F0 point input by one stage record after the first point. These are within-invocation observations, not frame-lifetime guarantees.

Synthetic face records start at `0x0C020118`, stride 8. The fixture packs the four point IDs into successive six-bit fields and checks those decoded IDs against original producer reads, callback input addresses, packet copies, and packet stores. For example, the first GT4 word is `0x000420C0`, decoding to `(0,3,2,1)`. The primary face sequence is `(0,3,2,1)`, `(2,3,1,2)`, `(1,2,3,3)`. It reorders points and repeats indices, including repeated corners. The culling discriminator puts a positive-area face first; its absence at packet commit leaves face records 1 and 2 associated with the two emitted packets.

The original GT4 face-word read is ENG1 `0x2E0F1`, with the record address at `[EBP+8]`. The FT4 boundary is `0x2E27B`. First-three-corner source-read order, area callback arguments, global XY reads, and packet destination writes are captured in the report. The fourth GT4 corner is loaded from stage memory at `0x2E183` and written at `0x2E18B`; FT4 uses `0x2E2C7` and `0x2E307`. Packet identity is confirmed at callback entry using `[ESP+4]` at `0x2C790` for GT4 or `0x2C770` for FT4.

The route extents stay distinct. GT4 executes selected instructions in the historical `2DF20` first-RET extent (`0x2DF20`, size `0x334`, RET `0x2E253`). A material mode byte of zero (`material+0x0E`) with dispatch flags 8 takes branch `0x2E0DA` to the separately traced FT4 tail `0x2E254..0x2E3A9` (size `0x156`, its own RET). Observing the tail does not enlarge the old first-RET function extent or change recovery state.

GT4 packets have tag `0x3C000000`, stride `0x34`; the FT4 packet has tag `0x2C000000`, stride `0x28`. The synthetic visit record sets winding word 3 and depth input word 256. Original stores in these cases target OT address `0x0C051384`, offset `0x4E1` (1249) from the synthetic OT base `0x0C050000`; the report preserves each store PC, value, and link chain. This is an observed fixture destination, not a general depth formula or frame-capacity claim.

The direct GT4 case and public slot-106 bridge case produce identical packet summaries (all four XY pairs, tags, links) and OT-head writes. The bridge's five captured B920 samples, including prep, match the original B920 observations. Its capture record belongs to the diagnostic bridge DLL; it is not an original callback service or a published runtime interface.

## Observer boundary and limits

The measured association uses emulator instruction-PC and memory-read provenance. A callback-only slot-106 observer has camera values and projected XY but no point ID. B6B0/B660/B5C0 also pass packed XY through globals and packet writes. Equal-XY points 0 and 3 prove a value-only join is ambiguous. B6F0 exposes a point-stage pointer, but that pointer alone does not carry the selected face index to a later packet corner.

The smallest additional producer-side observation is the pre-read boundary at ENG1 `0x2E0F1` (GT4) or `0x2E27B` (FT4): snapshot `[EBP+8]` and its face word, EDI's point-stage base, and ESI's candidate packet cursor. Confirm a commit at `0x2C790`/`0x2C770` with `[ESP+4]` and ESI; discard the face snapshot if culling prevents callback entry. The adjacent `eng1_geometry_association_observer.h` defines only this raw diagnostic record shape. It is not an existing callback ABI, detour, dispatcher, or GPU backend.

All point, mesh, material, visit, packet, and OT records are synthetic. The observed mesh selector is static table context, not an actor or dynamic object ID. The test proves no live object identity, frame continuity, real frame capacity, complete renderer behavior, full module recovery, or whole-binary identity. It does not modify originals, assets, recovery state, or canonical repository files.
