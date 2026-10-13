# ENG3 FT4 geometry association handoff

This package provides a repeatable, bounded original-x86 fixture for ENG3's FT4 producer at RVA `0x231A0`. It enters the measured producer directly with the argument roles statically identified at `0x901D`; it does not run the handler chain or resolve its visibility/bounds gate. The initial private full-chain prediction and its pre-execution correction are retained in the fixture's prediction file.

The fixture derives the ENG3 callback-table destination and the 1000-byte source-table identity from canonical `renderer_contract/evidence/native_interface_copy.json`, then verifies the original HERCULES.EXE and ENG3.DLL SHA-256 pins. It executes the original producer and the copied wrappers and HERC callback bodies reached by that producer. Unexpected code, unmapped reads, original code writes, or changed original-file hashes fail visibly. Synthetic inputs and bounded callbacks are listed in the JSON report.

The accepted run associates four source triplets with camera XYZ, projected XY and packet corners through actual point-pointer arguments and callback order. Points three and four reuse the same local scratch address, with the producer's X-word overwrite observed before the fourth callback. The fixture also records executed stream/object/material lookup reads, producer packet-field writes, raw tag `0x2e`, observed `0x28` cursor movement, and OT bucket insertion. These observations do not assign asset semantics to raw fields or prove live object identity, handler visibility, capacities, or rendering behavior.

Replay from a checkout root, using any copies of the hash-matching original files and a new ignored-work destination:

```powershell
& 'C:\Users\<user>\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' `
  'renderer_contract/tests/test_original_eng3_geometry_association.py' `
  --repo-root (Get-Location).Path `
  --exe '<path to HERCULES.EXE>' --eng3 '<path to ENG3.DLL>' `
  --output 'work/eng3_geometry_association_replay'
```

The test requires `--output` to be fresh and below the checkout's `work/`. It writes a compact report and full PC/read/write trace only there. The source fixture, prediction, public test helper, and canonical input evidence are hash-recorded in the generated report.
