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
| advance_planar_motion | — | 0x2b3f0 | 0x1f550 | 208 bytes, exact in both engines |
| advance_vertical_motion | — | — | 0x1f620 | 112 bytes, exact in ENG3 |
| remove_counted_value | — | 0x2b320 | 0x1f480 | 80 bytes, exact in both engines |
| bounds_overlap | — | 0x2a7a0 | — | 112 bytes, exact in ENG1 |
| advance_target_motion | — | 0x2b9a0 | — | 688 bytes, exact in ENG1 |
| apply_frame_motion_2d | — | 0x29160 | — | 112 bytes, exact in ENG1 |
| apply_frame_motion_3d | — | 0x291d0 | — | 144 bytes, exact in ENG1 |
| measure_absolute_vector | — | 0x2ac10 | — | 224 bytes, exact in ENG1 |
| measure_xz_vector | — | 0x2acf0 | — | 128 bytes, exact in ENG1 |
| planar_bounds_overlap | — | 0x2a4d0 | — | 80 bytes, exact in ENG1 |
| measure_relative_vector | — | 0x2ab30 | 0x1f280 | 224 bytes: ENG1 exact; ENG3 differs by two bytes |

Together with the earlier colour and sentinel routines, acceptance now covers
**seventeen distinct source functions and twenty-six exact module instances**: 2,704
bytes of unique contributions, 3,696 mapped target bytes including original
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
280-byte body plus eight NOPs is exact in ENG1 and ENG3. The adjacent planar
variant updates only x/y (194-byte body plus 14 NOPs, both engines); the vertical
variant updates only z (106-byte body plus six NOPs, ENG3). Both use the same
measured fields and clamp order. Adding these two functions to the source file
preserved the accepted full-motion bytes. These names describe coordinate
components; world-axis orientation has not been established here.

`counted_list.c` treats element zero as a signed count and searches slots 1
through count. It removes the first matching DWORD by shifting the suffix left,
then decrements the count. The scan and shift share an index, so a duplicate
later in the list remains. The old last slot is not cleared. Null, zero-count
and negative-count inputs make no writes. The original post-decrement lower
check remains even though a positive count is required to enter the loop.
Both engine copies have a 79-byte body and one NOP.

`bounds.c` tests six signed-short bounds at offsets 0 through 10. It combines
strict separation comparisons with bitwise OR, then negates the result;
face/edge/corner contact therefore counts as overlap. ENG1's complete 106-byte
body plus six NOPs matches the direct C expression. This proves the routine's
bytes and field access, not the original type name or callers' geometry policy.

The nearby point-containment routines (ENG1 0x2a810, ENG3 0x1f130) were also
examined. Natural grouped and accumulated comparison expressions under VC5
RTM `/O2 /Gy` did not reproduce their instruction ordering. Those scratch
experiments did not change acceptance or introduce compiler-steering code.

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

## Larger motion and geometry batch

The next six accepted functions add 1,376 complete contribution bytes, all in
ENG1. No byte-identical copies of these complete contributions were found in
TITLE or ENG3. The largest is `advance_target_motion`: 682 body bytes plus six
NOPs. Its low three control bits select per-axis target steering against the
positions at 0xd8, 0xdc and 0xe0. Moving away selects deceleration and sets the
corresponding return bit; otherwise acceleration applies. Negative speed is
clamped to zero and the direction byte is complemented. This is a full byte
complement: 1 becomes 254, while 255 becomes zero.

The y/z steering paths compare speed against **the x-axis limit at 0xac**, then
assign their own axis limit if exceeded. This asymmetry is preserved. Control
bits 3–5 independently apply the ordinary acceleration-minus-deceleration
updates afterwards; setting both groups can therefore update a speed twice.
Positions advance even with control byte zero. Nested direction/position
branches compile exactly; ternary and combined-condition hypotheses did not.
Only the low argument and return bytes are established here; several C argument
widths produced identical code, so the original declaration is not identified.

`frame_motion.c` covers indexed 4-byte x/y and 6-byte x/y/z signed-short steps.
Both functions gate on the signed field at 0x11e, read a table at 0xf0 and an
index at 0xf4, and scale steps to 16.16 coordinates. Explicit signed-short x
temporaries reproduce the original narrow-load/sign-extension sequence. The
2D form gets its z step from object offset 0x98. The 3D form captures the table's
z step before position writes and adds it for nonzero direction byte 0x96,
subtracting it for zero. That convention is opposite to ordinary z motion.
Negative steps retain the measured historical VC5 signed-shift behavior.

`measure_absolute_vector` shares the earlier distance and direction arithmetic
but stores magnitudes, rather than signed deltas, before sorting. The X/Z form
ignores y, writes its two-component estimate into `planar_distance`, sets the
other distance to zero, and does not add a direction bit when swapping x/z.
`planar_bounds_overlap` uses only x/y extents and deliberately ignores z.

Bounded scratch work also examined initializers at TITLE 0x94c0 and ENG3
0x1d8a0, the 240-byte row-buffer routine at ENG1 0x2b30, wrapped overlap at
ENG1 0x201b0, and target-vector measurement at 0x2ad70. They remain unaccepted.
Closest measured RTM `/O2 /Gy` initializer variants differed by 74 bytes;
wrapped overlap by eight; target-vector measurement by two shift immediates.
The row routine retained substantial register/loop differences. Natural field
types, grouped initialization, buffer indexing and historical scheduling flags
were investigated in ignored `candidates/large_batch/`; no padding, assembly,
dummy declarations or forced placement were added to obtain the accepted code.

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
python scripts/test_pilot.py                  # all registered semantic suites
python scripts/test_pilot.py --suite shared
python scripts/test_pilot.py --suite engine
python -m unittest discover -s tests -p 'test_*.py'
```

The shared harness passed 67,719 checks, including all 65,536 command words,
all motion sign combinations, clamping and untouched-memory checks, vector
octants, fractional negative coordinates and the signed-short minimum. New
checks compare planar/vertical composition with the independently accepted full
motion routine, exhaust short counted lists with duplicate values, and compare
bounds overlap against finite-set intersection, including degenerate and
touching intervals. Signed extremes are checked separately. The
new engine suite passed 31,510 checks, covering all 256 control bytes with
equal/unequal target relations, non-Boolean direction bytes, negative and unequal
limits, frame strides and inactive guards, vector sign variants, and planar
intersection with disjoint z bounds. The original 64 checks and 13
acceptance/recovery tests also passed. Semantic tests
execute only reconstructed code; fresh target-byte equality remains the proof.
