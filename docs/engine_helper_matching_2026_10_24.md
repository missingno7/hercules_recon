# Engine-helper matching follow-up, 2026-10-24

Thirteen existing ENG1/ENG3 functions gained FUNCTION_MATCH_RELOCATED acceptance:
3,456 complete compiler-contribution bytes, 62 direct calls, and five local
references covering one four-cell quadrant table. Game source, compiler flags,
and acceptance tools are unchanged.

| Module / source family | Newly accepted entries | Bytes |
| --- | --- | ---: |
| ENG3 interpreter handlers | `279d0`, `26b40`, `28a80` | 112 |
| ENG3 group spawning | `297d0` | 576 |
| ENG1 quadrant control | `2c090` | 240 |
| ENG3 multi-actor spawning | `10b10`, `11760` | 1,536 |
| ENG1 rendering list | `30430` | 192 |
| ENG3 interpreter/predicate handler | `13d30` | 64 |
| ENG3 actor initialization | `0ebe0`, `0ee40`, `0f650`, `056a0` | 736 |

The falsifiable question was whether these complete, existing code contributions
would pass the unchanged reference gate when their helper identities and local
table bounds were independently reviewed. All thirteen pass. Nineteen helpers
were grounded in their own earlier source/body and recorded entries, freshly
compiled for complete non-reference equality, and checked against original direct
entry calls outside all selected caller ranges. Twenty-one source/flag profiles
were freshly compiled for that review. Caller-implied destinations did not select
the bindings. Helper identity does not accept unresolved references inside the
helper or establish dependency closure.

## Interface and alias findings

ENG3's limited RNG bridge at `1fab0` calls through copied interface slot 37,
cleans up the argument, and returns. Its raw RNG bridge at `1fac0` tail-jumps
through slot 38. Existing interface evidence, the original copy destination and
250-word copy length, the original host table, and the host bounded-RNG division
were checked together. Reconstructed declarations differ in signedness/return
spelling, so this establishes the physical 32-bit cdecl argument/return behavior,
not original C types or symbol names. Seven limited and four separate raw RNG
calls remain in `0f650`; no source evaluation order was changed.

Shared-source symbol suffixes are not reliable module-local addresses. The
candidate `eng1.shared_31d30` calls `_title_0c7e0`, but ENG1 entry `c7e0` is an
actor music-control routine, not the required vector helper. That candidate was
excluded. The unchanged gate rejects the proposed binding despite zero
non-reference byte differences. Correct aliases in the accepted batch were
reviewed against each helper's own entry and source, including the shared
interpreter and rendering helpers.

## Local table and negative controls

ENG1 `2c090` has a 240-byte complete contribution. Original `cmp esi,3; ja`
bounds four table cells at offset `0xdc`; they select instruction offsets
`0x5b`, `0x7a`, `0x99`, and `0xb8`. The default is offset `0xd7`. The dispatch
operand and all four cells pass the existing native HIGHLOW/compiler DIR32
inventory checks. Padding remains included in the complete comparison.

Three controls fail as expected: substituting raw RNG for limited RNG, omitting
the final quadrant cell, and using the misleading TITLE suffix as an ENG1
callee identity. No verifier exception or new reference form was introduced.

## Verification and scope

Journaled promotion freshly checks 330 RAW/RELOCATED proofs, preserving all 317
earlier proofs. All 358 earlier accepted ledger records remain identical.
Only selected statuses/proofs and 156 diagnostic peer inventories changed.
All 43 legacy linked function checks pass; their 86 broader provenance drifts
remain the documented realignment limitation.
The remaining 1,640 supported masked rows pass in 201 sources with zero failures;
all 183 Python tests pass.

The ledger now contains 135 RAW rows, 193 RELOCATED rows, and 43 retained linked
rows: 371 unique rows / 33,856 bytes. Two linked rows retain raw proofs and must
not be counted twice. Matching does not establish whole-game behavior, original
source spelling, callee completeness, or module layout. No original host or game
execution was performed.

Independent helper/ABI evidence and explicit uses are frozen in
`evidence/engine_helper_matching_2026_10_24.json` and
`evidence/engine_helper_bindings_2026_10_24.json`. Verification counts and report
hashes are recorded in `evidence/engine_helper_followup_2026_10_24.json`.
Detailed builds and controls stay in ignored work/. Recovery.json alone grants
acceptance. Recheck with `scripts/match.py verify`, `scripts/similar.py verify`,
and the Python test suite; promotion is a completed transition.
