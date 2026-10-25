# Actor-call matching follow-up, 2026-10-24

Thirteen existing ENG1 functions gained FUNCTION_MATCH_RELOCATED acceptance:
2,608 complete compiler-contribution bytes, 61 direct calls, and 17 local
references covering two switch-table dispatches and all 15 table cells.
Game source, compiler flags, and acceptance tools are unchanged.

| Source family | Newly accepted entries | Bytes |
| --- | --- | ---: |
| Actor continuation | `21c00` (`initialize_spinning_particle`) | 272 |
| Boss unit | `246c0` | 192 |
| Status panel | `33910`, `35a30` | 368 |
| Early level actors | `8dd0` | 64 |
| Player unit | `363e0` | 528 |
| Level actors | `cb80`, `d190`, `fd00` | 656 |
| Player scene | `3a340`, `3aa10`, `3aaf0`, `3c230` | 528 |

The falsifiable question was whether these existing complete code contributions
would pass the current gate after independent helper identities and local switch
bounds were supplied. All thirteen pass. Helpers were selected from their own
prior identities and source declarations, then freshly compiled and compared
at their independently recorded entries. Caller-implied relocation destinations
did not select the external bindings. The semantic name `random_limit` retains
the previously reviewed slot-37 host RNG alias and does not claim an original name.

## Switch-table evidence

For `21c00`, original `cmp eax,3; ja` bounds a four-cell table at contribution
offset `0x100`. Its entries point to offsets `0x3a`, `0x42`, `0x4a`, and `0x52`,
matching frame selections 2, 3, 1, and 12. The last case also changes the owner's
parameter to 12. The owner write, attachment, and nine separate RNG calls remain.

For `3c230`, original `cmp eax,10; ja` bounds eleven cells at offset `0xd0`.
Opcode 4 and out-of-range opcodes share the default destination. The other cells
retain the original velocity/rate writes, script-pointer indirection, stop return,
state write, and frame-selection-before-interpreter ordering. Countdown behavior,
negative-delay iteration, and live script reads were not rewritten.

Every cell points to a decoded instruction boundary in its own complete function.
The existing gate checks the original HIGHLOW inventory against all compiler
DIR32 fields, including the two dispatch operands. Wrong-case and omitted-final-cell
negative controls both fail. No new reference form or verifier exception was added.

The pinned objdiff adapter was exercised on fresh script-handler builds alongside
the original-backed residual report. Its candidate equality was diagnostic only;
the explicit table/reference comparison established relocated acceptance.

## Verification and scope

Journaled promotion freshly checks 317 RAW/RELOCATED proofs: the 304 earlier
proofs and thirteen new ones. All 43 legacy linked function checks also pass.
Their 86 broader provenance drifts remain the previously documented realignment
limitation. Every one of the 345 earlier accepted ledger records is unchanged.
The remaining 1,653 supported masked rows pass in 201 sources with zero failures;
all 183 Python tests pass.

The ledger now contains 135 RAW rows, 180 RELOCATED rows, and 43 retained linked
rows: 358 unique rows / 30,400 bytes. Two linked rows retain raw proofs and must
not be counted twice. Matching does not establish whole-game behavior, dependency
closure, original source spelling, or original module layout.

Independent identities, original switch guards, and explicit COFF uses are in
`evidence/eng1_actor_followup_matching_2026_10_24.json` and
`evidence/eng1_actor_followup_bindings_2026_10_24.json`. Summary results and report
hashes are in `evidence/actor_call_followup_2026_10_24.json`. Detailed builds,
negative controls, and disposable packets stay in ignored work/.

To recheck the accepted state, use `scripts/match.py verify`,
`scripts/similar.py verify`, and the Python test suite. Promotion is a completed
one-time transition, not a command to rerun for verification.
