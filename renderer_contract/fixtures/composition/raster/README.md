# Bounded AC60 FT4 raster candidate

This source-only candidate models one connected synthetic path: live AC60 order-table consumption, FT4 state application, the original D420 direct-rectangle predicate, CAB0's indexed texture-to-CPU-surface loop, followed by the seeded remap request, unlock, presentation request, and controlled DIB/flush endpoints. It reuses the public pointer-free `CompositionEvent` / `CompositionTrace` API. The pixel write is recorded as `COMPOSITION_EVENT_SCREEN_SPACE_GRAPHIC` in the CPU-surface domain with `semantic_owner=UNCLASSIFIED`; the kind does not imply a world/UI layer.

The C subset derives the destination rectangle from D420's checked AC60 lanes and obtains UV origin from lane 0. For every accepted pixel it computes `(V + row) * 256 + U + column` in the synthetic texture and `(Y + row) * pitch + X + column` in the indexed CPU view, then delegates CAB0's zero-skip rule to the shared pixel helper. The shifted/resized `(4,5,3,2)` rectangle at UV `(2,1)` is the discriminator against fixed offsets. The model rejects nonrectangular and asymmetric-UV packets before events, provider calls, or surface mutation.

The domain remains deliberately finite: FT4 command/tag `0x2c`, unblended, tpage/CLUT zero, modulation byte `0x64`, texture token `0x96e6e0`, full in-bounds rectangles at most 8x8, texture stride 256, and CPU-surface pitch 320 for a 320x240 view. No generic fallback, clipping, palette conversion, asset texture, actual GDI call, game process, or gameplay chronology is included. The asymmetric UV original invocation reaches the generic D60C path but the model does not implement it. The original `0x70c0` remap call requests `0xff`; seeded guard `0x80` takes `0x70d4 -> 0x70db -> 0x7132`, skipping loop `0x70dd..0x7131`. The model therefore records a remap request, not pixel remapping.

## Fresh audit

Run from a clean checkout with a new, empty output path directly below repository `work/`. The audit does not read saved native receipts or prior reports. It executes three unchanged-original x86 fixtures (two positive rectangles and one asymmetric-UV falsifier), compiles the C source with strict GCC C89 and pinned VC5 `/Za /TC /W3 /MT`, runs base and offset cases under both compilers, compares fresh event/pixel/raw-input signatures to their fresh native receipts, and verifies C rejection of near-shape inputs. All generated files go only into the selected fresh output folder.

```powershell
$repo = (Resolve-Path .).Path
$py = 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
$exe = 'D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE'
$deps = 'C:\tools\hercules\renderer-oracle-python'
$analysisDeps = 'C:\tools\hercules\python'
$gcc = 'C:\msys64\mingw64\bin\gcc.exe'
$vc5Root = 'C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953'
$runner = Join-Path $repo 'renderer_contract\fixtures\composition\raster\tests\audit_composition_raster.py'
$out = Join-Path $repo 'work\composition_raster_audit_fresh' # use a new empty path on each run
& $py $runner --repo-root $repo --exe $exe --output-dir $out --python $py `
  --deps $deps --analysis-deps $analysisDeps --gcc $gcc `
  --vc5-cl (Join-Path $vc5Root 'bin\cl.exe') --vc5-root $vc5Root
```

The output folder must not exist or must be empty, must resolve below `work/`, and may not overlap `work/discs` or `assets`. The original EXE is read-only and accepted only at SHA-256 `587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`. In the managed Windows environment, launching the historical VC5 process can require the approved elevated process context to work around Windows error 623; its build outputs remain restricted to this fresh `work/` directory.

## Evidence and limits

The native runner checks the pinned original image and emits code-span hashes with each new receipt. Relevant spans are D420 `0xd420..0xdd70` (SHA-256 `4645c60cf0a357ed4aae074c3487a2755384852d0874e2f12b352b994544e07e`), CAB0 `0xcab0..0xcc45` (`5ca22ca070b69abaf98a0da5c794b0642cf816026b7ff87c794d6a82f6d9c71f`), and AC60 `0xac60..0xb2f0` (`f7724c04bef582e378b183a12550ba841df1a6fa3660be51da71154aa5c525af`). D420's four-lane rectangle/UV predicate is also represented by `hercules_quad_dispatch.h`; AC60 packet-to-lane provenance comes from `hercules_staging.c::stage_quad`; CAB0 samples and zero-skip behavior use `hercules_pixels.c`.

The base native case writes offsets 322, 641, and 642. The shifted/resized case reads six distinct nonzero texels at texture offsets `0x102..0x104` and `0x202..0x204`, writing CPU-surface offsets 1604..1606 and 1924..1926. GCC and VC5 results match each positive native event sequence, surface-byte diff, and raw order-table/packet bytes. These are synthetic controlled buffers and external service providers. They establish neither general rasterization nor a complete visible-writer inventory, original screen ownership, layer identity, final display success, nor full-frame or gameplay equivalence.
