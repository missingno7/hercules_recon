# Grid-scan compositions, 2026-10-25

The existing guarded `20b50` source and a private native-body draft both agree
with the original in 725 synthetic cases, for 1,450 source comparisons. Each
scan executes its real `20990` stream decoder. This expands the previous four
recorded grid comparisons without OS loading or original host/game startup.
No new function is accepted: the ledger remains at 408 functions / 39,296 bytes.

Run `python scripts/eng1_grid_scan_differential.py` for canonical source, or add
`--candidate candidates/grid_scan_2026_10_25/grid_scan.c` for the current ignored
compiler experiment. The tool freshly compiles the canonical scan, decoder and
existing coefficient owner with the pinned historical compiler. Fixture-only
globals supply synthetic readable row/header storage. Unicorn confines native
execution to the observed scan/decoder spans and source execution to the linked
fixture functions; reaching the unsupported-path diagnostic fails the case.

The corpus covers positive group counts 1..16, widths 1..8, several x extents
and base offsets, coordinate WORD wrapping, clamped layers, active-row and
exclusive-extent exits, empty cell tags, zero relative records, mirrored cells,
positive and negative signed relative program offsets, decoder sentinels,
constant samples and linear interpolation. Every decoder program has tag zero;
the cell tags that select these programs are a separate field. Other compressed
program families remain outside this corpus.

Of the 725 cases, 336 call the real decoder (1,030 native invocations), and
371 read grid cells. Reads cover all 16 synthetic layer slots; 36 decoder cases
use negative relative records and 237 cases read a one-past-width cell. The
remaining cases exercise earlier exits.

Full return values, decoder argument traces, row-read order and complete fixture
data agree. Return addresses, ESP, callee-saved registers and caller guards are
checked. Incoming argument-copy values are excluded because the original reuses
those stack slots; fixture pointers do not alias those copies.

Two explicit private mutants fail:

- Seeding the reverse scan from the forward sample produces `0x00280000`
  where the original returns zero. The neighboring cell deliberately has a
  different decoded sample, so the error cannot hide behind an equal result.
- Skipping the one-past-width cell produces `0x7fff7fff` where the original
  returns `0x00017fff`. That cell is explicitly readable in the synthetic row.

The compiler question remains open. The initial direct C draft compiled to
496 bytes; unsigned coordinate operations and early flip extraction reduced it
to 464. Reusing coordinate copies and explicitly materializing the lower-result
stack copy reaches the observed 480-byte extent, but 358 non-reference byte
differences and different relocation positions remain. C++ mode alone, combining
a local lifetime and declaration ordering did not resolve them. The implied
masked symbol map is invalid while those instruction fields disagree; it is not
reference evidence. The private draft remains under ignored candidates/, with
no production replacement or diagnostic staging.

The 16-layer backing stores, row padding, headers and program buffers are
fixtures, not native complete-capacity proofs. Canonical null-pointer,
nonpositive-count, selected-index and width diagnostics remain unchanged. No
valid-domain observation licenses removing them or accepting indexed BSS
references. All 367 accepted strict proof inputs remain unchanged and their
reference evidence pins were rechecked. All 218 Python tests pass.

The compact frozen receipt is
`evidence/grid_scan_composition_2026_10_25.json`; detailed cases and negative
controls remain under ignored `work/grid_scan_2026_10_25/`.
