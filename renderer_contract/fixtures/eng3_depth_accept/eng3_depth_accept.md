# ENG3 controlled interval and depth decision

This candidate adds a rerunnable native fixture for the controlled positive path through original ENG3 `0x24A80`. It uses only the public ENG3 fixture utilities in `renderer_contract/fixtures/eng3_visibility_source/run_eng3_visibility_source.py`; it has no import into phase-19 or phase-20 scratch. The adjacent [prediction and falsifier](prediction_falsifier.md) is the v4 prediction written before that experiment and is hash-pinned by the runner.

The runner first executes original `0x204C0` with lower `10` and extent `10`, requiring the exact row bytes `00 00 00 00 0A 00 00 00 14 00 00 00`. Each native case copies the entire `0x3E8`-byte HERC callback prefix from EXE RVA `0x361B8` to ENG3 VA `0x100407A0`; slot 5 therefore points to the original file-backed table at VA `0x437480` / RVA `0x37480`. The runner hashes the complete EXE and ENG3 files, each executed code span, the full prefix, and the 8192-byte table. It directly calls `uc.mem_read` inside read hooks to measure operands. The hook metadata value is recorded only as a diagnostic because a historical receipt read zero from that field while the mapped file contains `0x1000` at table offset zero.

Coordinate 9 (`object+0 = 0x04800000`) returned `EAX=0xffffffff` in 18 instructions before helper `0x24BB0`; it did not invoke geometry callbacks. Coordinate 10 (`object+0 = 0x05000000`) reached the helper and returned `EAX=0` after 440 instructions. Its controlled 16-byte record at stride index 10 has byte `+0xF=5`; original ENG3 wrote WORDs `(0,0,640)` into source slot-4's EXE `.data` zero-fill target `0x00BA25C0`, and original HERC slot 101 read the same three values. With zero-angle setup, center `(0,0)`, focal `256`, and three controlled Q12 scale factors `[4096,4096,4096]` at the original slot-165 inputs `+0/+4/+8`, original callbacks produced camera XYZ `(0,0,640)` and projected object WORDs `(0,0,160)`. These three DWORDs are measured scale-factor operands; the fixture asserts no matrix structure or runtime camera state.

The successful epilogue is separately byte-pinned at ENG3 `0x24AEF..0x24AF4` (`8B C7 5F 5E C3`, SHA-256 `cadc250d09720749067905ca17f6f4be7be66a84e4f6bfe9fef3bf140b943d19`). Reads from the table were measured from the original mapped file bytes; four of the eight table read-hook metadata values differed from those direct reads, and no non-file-backed read remained uninitialized. The report labels original span identity `STATIC_OBSERVED`, the earlier public C interval seam `BOUNDED_EQUIVALENT`, and the two new native decisions `BEHAVIOR_VERIFIED` as separate proof entries.

This does not bind the controlled row or geometry record to Cyclops or another runtime resource. It does not run the caller/object-list flags, `0x8DA0`, or packet producer `0x231A0`. The returned zero takes the non-`-1` edge at the statically observed caller comparison, but does not establish accepted-to-emission or emitted geometry. The earlier phase-19 receipt at `work/renderer_phase19/eng3_prefix_tail/runs/positive_tail_v1.json` remains an unchanged historical record of the bad hook-value interpretation; the public static/C source-seam evidence remains untouched.

## Replay

From the repository root, provide the pinned read-only originals and a new output under `work/`:

```powershell
C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe `
  renderer_contract\tests\audit_eng3_depth_accept.py `
  --repo-root . `
  --exe D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE `
  --eng3 D:\Prog\hercules_recon\work\discs\pc_install\ENG3.DLL `
  --output work\eng3_depth_accept\fresh_receipt.json
```

The compact measured receipt is in [evidence](../../evidence/eng3_depth_accept_phase20.json). Runtime outputs are generated only under ignored `work/`; no table bytes are copied into the candidate. The runner rejects output redirects, existing outputs, assets targets, original-file parents, span/hash mismatches, unexpected code, invalid memory, uninitialized non-file-backed image reads, and controlled-input writes.
