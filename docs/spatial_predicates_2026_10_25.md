# Spatial predicates and callback ABI, 2026-10-25

Four functions gained FUNCTION_MATCH_RELOCATED acceptance: `2a710` (144 bytes),
`2aa30` (128), `19440` (64) and `d240` (128). Their complete contributions total
464 bytes with fourteen independently verified references.

The ledger contains 390 unique accepted rows / 36,128 bytes: 135 RAW,
212 RELOCATED and 43 retained linked rows. All 386 earlier accepted categories,
targets, extents, profiles, compiled-byte identities and reference maps remain
intact. Two linked rows retain RAW proofs and are counted once.

Two missing predicates now have real source in
`src/eng1/behavioral/actor_spatial_predicates.cpp`. `2a710` takes the area first
and actor second, obtains actor bounds in mode zero, then applies six signed
WORD comparisons; touching bounds overlap. `2aa30` checks the pipeline WORD
and screen flags before signed coordinate comparisons with unsigned low-WORD
margins. A nonzero pipeline WORD returns before reading the actor.

Identical bounds logic compiled as C has 62 non-reference byte differences.
Reordering OR terms, explicit accumulation and member/array representations
did not change that residue. C++ with C linkage reproduces both complete
instruction contributions. A final control removes only the language wrapper
from identical source and confirms the C residue. This is a measured reproducing
context, without proving historical source language or TU identity.

All 1,079 isolated original/source cases pass in pinned Unicorn: 237 bounds
cases and 842 screen cases. Checks cover exact returns and argument transcripts,
signed extremes, touching/invalid bounds, failed producer calls, live area
mutation, high margin bits, guarded null actors, full synthetic input regions,
stack guards, callee-saved registers and unchanged timer storage. The actor-bounds
producer is explicitly modeled. No game process or actual resource chain runs.

The screen predicate's two-byte COMMON owner at `71376` was independently
compiled. Its native setup anchor stores five. All seven exact-cell references
access WORDs and account for all seven native HIGHLOW references to this cell.
A neighboring-cell control and wrong-helper control both reject. Existing pool
and collision-pointer owners were reused unchanged.

Native callers `128e` and `155e` establish area-first, actor-second arguments.
The older behavioral callbacks reversed them. Both calls and their declaration
are corrected. The expanded source-spy fixture passes current source and rejects
an exact reconstruction of the old source. Every other function and all non-code
contributions in that unit remain identical.

Because that file contains accepted `1e00`, `scripts/reprove.py` checked all
345 then-existing strict proofs before and after the edit. Accepted compiled
hashes, target hashes and binding maps remain identical. Original manifests stay
immutable; one descendant pins the parent, explicit review and new source hash.
Context metadata changed for 157 existing relocated proofs, with no acceptance
or status change. See [the refresh procedure](context_refresh_2026_10_25.md).

Final promotion checks all 349 RAW/RELOCATED proofs in 92 fresh compiler calls
and all 43 legacy linked checks. Their existing 86 broader provenance drifts
remain. All 218 Python tests and 1,632 supported diagnostics in 199 sources pass.

The genuine ENG1 graph now has 948 mapped providers out of 1,231 diagnostic
spans, with zero duplicate providers or generated placeholders. Three obsolete
placeholder names were removed. Linking still fails with 757 unresolved symbols:
447 functions and 310 data/untyped names. Dependency closure and whole-game
completeness remain unproved.

Independent evidence is in `evidence/spatial_predicates_recovery_2026_10_25.json`,
`evidence/spatial_predicates_references_2026_10_25.json`,
`evidence/spatial_predicates_bindings_2026_10_25.json`, and
`evidence/default_callback_abi_review_2026_10_25.json`. The final frozen receipt is
`evidence/spatial_predicates_progress_2026_10_25.json`. Recovery.json alone grants acceptance.
