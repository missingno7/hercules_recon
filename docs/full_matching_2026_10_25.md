# Full-matching continuation, 2026-10-25

The active goal is faithful function matching across the original PC first-party
code. Recovery.json remains the acceptance authority; source completeness,
dependency closure and runtime behavior remain separate. The goal is not complete.

## Accepted caller entries

Nine existing functions gained FUNCTION_MATCH_RELOCATED acceptance: 1,008 complete
contribution bytes and thirteen direct references. This brings the ledger to
380 unique accepted rows / 34,864 bytes: 135 RAW, 202 RELOCATED, and 43 retained
linked rows. Two linked rows also retain raw proofs and must not be counted twice.

| Family | Entries | Bytes |
| --- | --- | ---: |
| ENG1 / ENG3 middle resets | `initialize_middle_actor`, `reset_eng3_middle_actor` | 352 |
| Host sound callers | `10010`, `0c140` | 64 |
| ENG3 height followers | `145d0`, `15340` | 368 |
| ENG1 render setup | `320d0` | 96 |
| ENG1 floor / state update | `08480`, `112d0` | 128 |

Eleven referenced entries were reviewed independently using their own original
bodies, ABI and earlier source/evidence, plus original direct callers outside
the selected caller ranges. Some helpers have established bounded semantic
evidence but still differ from the original compiler output. Their callers'
acceptance identifies the referenced entry; it does not accept those helpers'
implementations or globals. The stop/pause and cross-module reset substitutions
both fail the unchanged gate despite zero caller non-reference differences.
The compact-pool wrapper remains diagnostic because the chosen outside-caller
witness requirement was not met. No gate or source change produced these nine
acceptances.

All 371 earlier accepted records remain identical. Promotion checked 339 complete
RAW/RELOCATED proofs, preserving all 330 earlier proofs, plus all 43 legacy linked
function checks. The 86 existing broader linked provenance drifts remain unchanged.
The reference manifest and independent review are frozen in
`evidence/caller_entry_bindings_2026_10_25.json` and
`evidence/caller_entry_matching_2026_10_25.json`.

## Newly recovered player-mode source

Eleven missing entries now have real C bodies in
`src/eng1/behavioral/player_modes_remaining.c`: `18a10`, `18ba0`, `18c30`, `18ed0`,
`19140`, `191e0`, `19260`, `19290`, `193f0`, `19480`, and `19c20`. They cover
1,536 original bytes. Their source supplies fixed-width field accesses, WORD/DWORD
wrapping, short-circuit call order, live pointer reloads, attached-chain traversal,
direction changes and original coordinate updates. It supplies no guessed opaque
image payload or nested service behavior.

Seven complete contributions reproduce all non-reference bytes. They were staged
as CODEGEN_SIMILAR diagnostics, not accepted matches. Four remain nonmatching:

| Entry | Complete extent | Non-reference differences |
| --- | ---: | ---: |
| `19260` | 48 | 6 |
| `19290` | 352 | 229 |
| `193f0` | 80 | 16 |
| `19480` | 464 | 232 |

Natural source changes exposed explicit pointer sequencing, a WORD postincrement,
and branch-specific stores; no flags, padding or dummy declarations varied. A
C++ probe motivated by the neighboring recovered unit preserves the same seven
equal functions and leaves all four residuals unresolved. That probe is negative
context evidence, not a new source configuration. The unit remains editable for
continued context recovery.

The new `scripts/eng1_player_modes_differential.py` compares fresh VC5 output with
unchanged original handler instructions in the pinned Unicorn runtime. Its normal
linker-built fixture intercepts every external service and rejects execution
outside the selected body. All 360 finite cases agree on ordered calls/arguments,
ordered writes, four full actor records, selected global cells, live callback
pointer changes, aliasing, timer/direction/query boundaries, and cdecl guards.
External services are modeled; this does not validate their bodies or whole-game
behavior. No original DLL is loaded or game/host process launched.

All eleven entries were removed from the diagnostic placeholder manifest. The
real ENG1 source graph now compiles 944 mapped providers, with zero duplicate
providers and no generated placeholders. It still fails its genuine link with
760 unresolved names: 450 function and 310 data/untyped. Implementation coverage
is not matching coverage. Independent source and fixture results are in
`evidence/eng1_player_modes_2026_10_25.json`; final verification and hashes are in
`evidence/full_matching_progress_2026_10_25.json`.
