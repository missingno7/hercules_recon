# Bounded native packed-UV feedback contract

`tests/test_original_route_uv_feedback.py` executes original HERCULES.EXE CPU
instructions for three finite pixel-loop routes on one synthetic clipped
319-pixel row. It covers D420 to CC50 row 3, E680 mode `0x80` with selector 2,
and E680 mode `0x7F` through DD70's generic shaded loop. Paired runs vary the
source byte or one selected lookup result, then each route runs an all-zero
texture case. The runner checks the pinned EXE, original instruction spans,
register landmarks, all source and table access order, destination writes, and
the all-zero skip behavior. It accepts an explicit receipt path anywhere below
the repository's `work/` directory.

The observed byte lifetimes differ by route:

- **CC50 palette:** The source byte is loaded into `DL`; the palette result
  replaces `DL` and is written to the destination. UV stepping uses `EAX`,
  `EBX`, and `ECX` (`BL` carries U and `CH` carries V), with `AX` as the loop
  count. Each source address rebuilds `DL`/`DH` from `BL`/`CH`. A zero `DL`
  skips palette and destination accesses. Across the controlled source and
  palette pairs, the source/palette byte changes `EDX` and the output only;
  all 319 later sample addresses remain equal.
- **E680 mode `0x80`, selector 2:** The source byte is loaded into `DH`, and
  destination and blend bytes use `DL`. UV stepping uses `EAX`, `EBX`, `ECX`,
  and the corrected texture pointer in `ESI`; `AX` counts iterations. The next
  sample rebuilds `DH` from `BL` and clears `DL`. A zero `DH` skips destination,
  blend, and output accesses. Raw-source and blend-result pairs change only
  color-path state/output, leaving the tested 319 sample addresses unchanged.
- **E680 mode `0x7F` through DD70:** The raw source and shade result occupy
  `DH`; destination and blend bytes use `DL`. The packed stepping and `AX`
  count match the tested mode-`0x80` route. The next iteration reconstructs
  `EDX` from the UV lanes. A zero source skips shade, destination, blend, and
  output accesses. Raw-source, shade-result, and blend-result pairs leave the
  tested sample addresses unchanged.

The bounded D60C contrast is documented in
[packed U/V feedback](packed_uv_recurrence.md), with its compact native receipt
[`native_packed_uv_feedback.json`](evidence/native_packed_uv_feedback.json).
That separate 319-sample D60C row-zero fixture shows a sampled nonzero texel
changing a later V byte at `(193,1)`. The source-backed C89 state replay and
its receipt [`native_packed_uv_state.json`](evidence/native_packed_uv_state.json)
verify the captured D60C register transitions only. The older
[`packed_uv_counterexample.json`](evidence/packed_uv_counterexample.json) is a
different 36-sample fixture whose two texel runs have equal UV streams. These
finite fixtures coexist and do not imply a universal recurrence.

The test uses public quad/polygon oracle helpers and the repository's pinned
Unicorn 2.1.4 / analysis dependency paths. It maps original PE sections at the
true image base, leaves original code unchanged, overlays only synthetic
texture, table, stack, and surface data, and stops at the existing harness
sentinel. It rejects unexpected accesses, unknown control flow, code mutation,
or instruction-budget exhaustion. The fixture uses a synthetic 512-byte
surface stride so the full row is observable.

This is finite CPU evidence for the listed routes, mode, selector, and fixture.
It does not establish other E680 modes/selectors, generic packet or clipping
behavior, full-raster or pixel-coverage equivalence, host integration, GPU
behavior, gameplay, or whole-renderer equivalence.

Reproduce the three finite CC50/E680 route comparisons from the checkout root
with the pinned Python runtime and dependencies. The runner writes its
requested receipt only below ignored `work/`:

```powershell
$py = 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $py -B renderer_contract\tests\test_original_route_uv_feedback.py `
  --exe 'D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE' `
  --original-root . `
  --deps 'C:\tools\hercules\renderer-oracle-python' `
  --analysis-deps 'C:\tools\hercules\python' `
  --receipt work\route_uv_feedback\replay.json
```

The compact package receipt is
[`native_route_uv_feedback.json`](evidence/native_route_uv_feedback.json).
It pins the original code spans, controlled route cases, sample and lookup
events, zero-source behavior, tool/runtime provenance, and finite proof scope.
The command does not read or depend on the earlier private receipt.
