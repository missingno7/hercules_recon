# TITLE native behavioral differential

This fixture checks whether the reconstructed `set_decimal_digits`
matches the original routine's state changes even though its current codegen
differs. A mismatch in any byte of the four complete 0x58-byte sprite records
falsifies that prediction. The same process also compares `make_colour` and
`sentinel_count` as practical recovered controls.

Run from the repository root with:

```powershell
python scripts/title_differential.py
```

The script verifies `scripts/match.py`'s pinned `msvc5_rtm` toolchain, freshly
builds a 32-bit fixture from `calibration/title_digits.c` and
`src/title/pilot.c`, and runs it with the path to `assets/pc/HERCULES/TITLE.DLL`.
The fixture loads that DLL with `LoadLibraryA` for the test only and calls the
three original routines at their recovery-pinned RVAs using `loaded_base + RVA`;
they are not exports (`TITLE.DLL` exports only `PC_DLLEngineMain`, ordinal 1).
The reconstructed functions are statically linked from the source files above
and do not call or import the original DLL.

`set_decimal_digits` compares every byte of four complete records over 15
pointer-alias partitions, the listed positive and negative arithmetic
boundaries including `INT_MIN`/`INT_MAX`, and 2,048 deterministic random values.
The pseudo-random generator starts at `0xD1FF3A71`. `make_colour` exhausts all
65,536 intensity values for flags 0, 4, and `0xffffffff`, then checks 2,048
deterministic random pairs. `sentinel_count` checks lengths 0 through 128 and
2,048 deterministic random arrays, including unchanged input bytes.

On a mismatch, the first divergence is printed with the routine, test ordinal,
input (and alias map / state byte for decimal digits), plus original and
candidate values. Re-running the same command reproduces the same first
divergence. The exact compiler and fixture commands, all input hashes, oracle
PE/export/import metadata, stdout, and result are in
`work/title_behavioral/differential/report.json` and its logs. The measured
scope is behavioral comparison for these inputs only; it grants no byte match,
source identity, or whole-module equivalence.

The oracle SHA-256 is `b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b`.
`docs/inventory.md` records the matching asset's PE metadata and says the
installer DLLs are byte-identical to the immutable asset copies; it also notes
that the supplied PC disc is a later repack, so retail-disc authentication is
not established. The report records that inventory file's hash, the oracle's
PE/export/import metadata, and the pinned compiler archive/tree/key-file hashes.

The separately built DLL is also compared directly against the original:

```powershell
python scripts/title_behavioral.py
python scripts/title_linked_differential.py
```

The fixture resolves internal providers through the new link map and the
original RVAs. It compares `0x94c0` reset over 1,024 randomized guarded records
with alignments 0 through 3, `0x4fe0` initialization over 64 randomized 3,072-byte
states including untouched fields, and cold null/bad-signature entry behavior
including the entire copied interface and first-use flag. The two DLLs have
distinct test-only filenames and module handles. Reports and commands are in
`work/title_behavioral/differential/newDLL/report.json`; the replacement itself
has no dependency on this oracle. Both comparison suites passed on 2026-10-11.
