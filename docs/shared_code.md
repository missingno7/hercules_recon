# Shared engine recovery

Continuation milestone, 2026-10-07. `recovery.json` is authoritative; the table
below summarizes the measured evidence in `evidence/shared_functions.json`.
Names and filenames are provisional semantic groupings, not recovered symbols.

| Function | TITLE RVA | ENG1 RVA | ENG3 RVA | Full contribution |
|---|---|---|---|---:|
| clear_command_high_bit | 0x9770 | 0x283a0 | 0x1db80 | 64 bytes, exact in all three |
| unlink_12c | 0x97b0 | 0x294b0 | 0x1e350 | 64 bytes, exact in all three |
| unlink_0e4 | 0x98a0 | 0x29620 | 0x1e4c0 | 80 bytes, exact in all three |
| advance_object_motion | — | 0x2b4c0 | 0x1f690 | 288 bytes, exact in both engines |
| measure_relative_vector | — | 0x2ab30 | 0x1f280 | 224 bytes: ENG1 exact; ENG3 differs by two bytes |

Together with the earlier colour and sentinel routines, acceptance now covers
**seven distinct source functions and fourteen exact module instances**: 816
bytes of unique contributions, 1,520 mapped target bytes including original
alignment. Repeated engine copies do not count as additional reconstructed
algorithms. Every instance was freshly compiled and passed the same strict gate;
matching a byte pattern alone did not change recovery status.

The two unlink routines retain their measured `src/title/pilot.c` translation-unit
context. Newly reconstructed shared functions live under `src/shared/`. The
identical copies support shared code/layout; they do not identify historical
translation-unit names. No complete module has been linked or reproduced.

## Source and behavior evidence

`object_commands.c` checks the flag at offset 0x50 and the pointer slot at 0x128,
then conditionally clears the high bit of a 16-bit command word. The code range
is 0xb780 through 0xb7ff. The exact source uses `code & ~0x7f`: VC5 emits the
original unsigned-short promotion and two AND operations. Using `code & 0xff80`
is semantically equivalent for these inputs but emits different bytes. The
inner pointer is not null-checked because the original does not check it.

`motion.c` applies acceleration minus deceleration to three speeds at offsets
0xa8, 0xb8 and 0xc8, clamps each first to zero and then its axis limit, and updates
the three positions according to direction bytes at 0x94–0x96. The clamp order
is preserved even when a negative limit produces a negative result. The whole
280-byte body plus eight NOPs is exact in ENG1 and ENG3.

`relative.c` computes signed 16-bit deltas from 16.16 position differences,
classifies direction, and forms planar and spatial distance approximations.
The observed NOT/INC pairs are reproduced by `~n + 1`; replacing that expression
with unary negation emits different code. A 32-bit swap temporary accounts for
the original sign-extension instructions. The original signed-short narrowing,
including the unusual -32768 absolute-value edge, remains intact.

ENG3's counterpart differs only at contribution offsets 0x91 and 0x97: the two
shift immediates 2 and 3 exchange positions. The two additive terms are present
in both. The same source remains two bytes away under RTM and SP2, and therefore
**ENG3 is CODEGEN_SIMILAR, not FUNCTION_MATCH**. Whether the difference reflects
source spelling or translation-unit/compiler context is unresolved.

## PSX counterpart for decimal display

The unresolved PC decimal routine at TITLE RVA 0x1a2c0 has a strong counterpart
at PSX v1.0 address 0x80048c48 and v1.1 address 0x80049118. Each MIPS function is
388 bytes including the return delay slot. Their decoded instructions agree
after normalizing internal jump addresses; raw hashes remain distinct.

The functions share five arguments, signed division by 1000/100/10, frame writes
at offset 0x34, addition of 90 to all four frames, and bit-31 flag updates at
offset 0x54 for thresholds 1000/100/10. The MIPS data flow retains separate
place-value products. This corroborates semantics and partial struct offsets;
it does not fix the PC code-generation mismatch or promote a PSX match.

## Reproduce and test

```powershell
python scripts/shared_evidence.py --output build/shared_evidence.json
python scripts/archaeology.py function eng1.measure_relative_vector
python scripts/match.py verify --report build/verification.json
python scripts/test_pilot.py                  # both semantic suites
python scripts/test_pilot.py --suite shared
python -m unittest discover -s tests -p 'test_*.py'
```

The shared harness passed 65,598 checks, including all 65,536 command words,
all motion sign combinations, clamping and untouched-memory checks, vector
octants, fractional negative coordinates and the signed-short minimum. The
original 64 checks and 13 acceptance/recovery tests also passed. Semantic tests
execute only reconstructed code; fresh target-byte equality remains the proof.
