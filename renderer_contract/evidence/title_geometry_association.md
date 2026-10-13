# TITLE mode-5 geometry association

Status: **TITLE_MODE5_NATIVE_PRODUCER_BOUNDED_PASS**.

The unchanged original TITLE+0x181e0 producer exposed four role-addressed signed-WORD XYZ inputs at native callback boundaries; the per-case receipt records any equal coordinate triplets explicitly. Slots 102/107/117 produced corners 0–2; slots 101/106/114 produced corner 3. The output packet, texture fields, OT link/insertion, and 0x28 cursor step are recorded per case. The current public C89 single-point model matched eight independently evaluated point outputs sampled at native batch/single callbacks; this is a valid positive case for that model, not general coverage of native batch math. Resource byte2=0x97 was compared with byte2=0 and its geometry/UV deltas are recorded explicitly. For the clear-byte case, same-XYZ role groups were [["batch_vertex_0", "batch_vertex_2"], ["batch_vertex_1", "single_fourth_vertex"]]; same-stack reuse was [{"pointer": "0xd00fdd8", "identities": ["batch_vertex_2", "single_fourth_vertex"], "xyz_words": [[36, 62, 0], [92, 62, 0]]}]. The identity key includes the callback role and time because TITLE reuses one local stack slot between the third batch input and the fourth single-point input. The runner requires a passing public projection report and validates its current harness, shared-width header, host ABI header, toolchain, and runtime hashes dynamically; paths are supplied at run time.

This is a finite Unicorn producer execution against pinned original bytes, not a claim about source identity, world units, general visibility, all TITLE modes, or pixels.

Receipt SHA-256: `98285bb9fce2dd8f6abb725e295f92be08a2c8168ed918d60e5924472e401a2a`.

## Public replay

Use hash-matching originals, the pinned tool directories, and fresh scratch
directories. First generate the single-point report with
[the projection runner](../tests/test_original_projection.py); its reproduction
is documented in [upstream_projection.md](../upstream_projection.md). Then run:

```powershell
& $pythonExe -B renderer_contract/tests/test_original_title_geometry_association.py `
  --repo-root (Get-Location).Path --exe $originalExe --title $originalTitle `
  --projection-report work/replay_projection/report.json `
  --output work/replay_title_geometry --vc5 $vc5Root `
  --deps C:/tools/hercules/renderer-oracle-python --analysis-deps C:/tools/hercules/python
```

The runner validates current public source/header/tool identities in that report
and compiles the current projection reference. It does not require any private
investigation source or fixed installation path. The compact shipped JSON binds
the root replay receipt above; full generated traces stay under ignored work.
