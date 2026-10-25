# Player animation storage and transition, 2026-10-25

Three missing animation owners now have typed source in
`src/eng1/behavioral/player_animation_programs.{c,h}`. Their 92 bytes have unique
complete native payloads, with extents derived from the actual pair/WORD
consumer formats rather than neighboring labels or padding. The unchanged
0x37390 transition client gains a 112-byte RELOCATED proof. Recovery.json now
records 473 distinct accepted entries / 47,392 bytes: 137 RAW, 293 RELOCATED /
33,184 bytes, and 43 retained linked rows. All 472 prior accepted records remain
identical, including their retained reference-anchor histories.

| Native RVA | Complete source owner | Format | Bytes |
| --- | --- | --- | ---: |
| 0x633e8 | g_player_program_633e8 | Eight frame/tick pairs and restart pair | 36 |
| 0x634b0 | g_player_program_634b0 | Duration, seventeen frames and restart | 38 |
| 0x634e0 | g_player_program_634e0 | Duration, seven frames and restart | 18 |

The pair interpreter increments its record cursor before reading a command.
Restart resets it to -1, so a new iteration begins at record zero. Each record
has a signed command and signed tick WORD; its four-byte stride is independently
observed in the native consumer and asserted by the compiled source type. The
terminal record includes its second WORD even though this restart path does not
read it. Full native/source payload equality verifies that field as well.

The WORD programs reuse the separately tested duration/frame/restart profile.
They restart the cursor at zero before reading frame WORD one. Each complete
compiled payload occurs exactly once in ENG1. Two proposed 36-byte programs at
0x63350/0x63378 are identical, and the six-byte payload at0x634d8 occurs seven
times. Those identities remain unbound; a convenient address or adjacent label
cannot resolve their physical ownership.

The actual 0x37390 client copies player position, invokes its current WORD
program, and, in state 0x23 on a loop result, installs 0x634e0 on the current glow
actor, resets its frame state and invokes it again. Its full unchanged historical
compilation passes complete instruction/padding and independent reference checks;
five wrong-reference controls reject. The helper entry and complete pointer
owners reuse earlier independently reviewed identities, without transferring
acceptance to the interpreters.

`python scripts/player_animation_differential.py` passes 306 guarded native/
source scenarios / 1,103 top-level calls: 201 pair calls, 518 WORD calls and
384 client calls. Nested interpreter/reset invocations are separately counted in
the runtime receipt. Cases cover complete cycles, stable cursor positions,
signed tick extremes, state gates, aliased glow input and transitions from the
longer WORD program to 0x634e0. They compare complete synthetic memory, explicitly
normalized program pointers, ordered stores, program/cell/table reads, return
values, argument frames, return stack and nonvolatile registers.

The real pair and WORD interpreters and reset helper execute. The pair
interpreter is context-sensitive: removing unrelated bodies preserves its 1,088
byte extent but changes 200 non-reference bytes. The fixture therefore compiles
the existing C++ unit unchanged, with fresh native equality checks. Code/read
guards permit only the reviewed interpreters, reset helper, client and complete
program extents. Unrelated functions/storage use private linker-only traps and
cannot execute or be read by a passing case. These are never production sources.
No compiler flags, body logic, instruction/object bytes or acceptance checks are
changed to make the fixture pass.

Seven new adversarial pair-format tests reject truncated records, missing or
misplaced restart, loops without frames, unsupported opcodes and invalid WORDs.
They also distinguish tick/ignored operand values from command opcodes. The full
suite has 301 tests. The full promotion passes 432 strict proofs and 43 retained
linked function checks; 86 broader linked provenance drifts remain.

The source plan adds only the new data owner. Original gameplay/storage units,
headers, profiles and core matching verifiers remain unchanged. The source-build
comparison and final census are recorded in
`evidence/player_animation_progress_2026_10_25.json`. References, runtime and
bindings are frozen in `evidence/player_animation_*_2026_10_25.json`; protect
them before future edits. These proofs establish the reviewed storage/consumer
boundary, not original C declarations, actor capacities, all-opcode interpreter
behavior, original OS startup or complete game/function matching.
