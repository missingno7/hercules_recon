# Collision-pointer reference matching, 2026-10-25

Three unchanged functions gained FUNCTION_MATCH_RELOCATED acceptance, covering
416 complete contribution bytes and 28 independently checked references:

| ENG1 entry | Bytes | References |
| --- | ---: | ---: |
| `19c20`, collision actor WORD reset | 16 | 1 |
| `bb90`, player motion transition | 192 | 9 |
| `2d320`, direction and frame selection | 208 | 18 |

The ledger now contains 386 unique accepted rows / 35,664 bytes: 135 RAW,
208 RELOCATED and 43 retained linked rows. All 383 earlier accepted records remain
identical. Two linked rows also retain raw proofs and are counted only once.

The collision pointer at `7217c` has one existing four-byte COMMON source owner.
Fresh compilation confirms that complete extent. Independent native normal setup
loads the normal-pool pointer at `153c4`, preserves EAX and stores it at `15403`
before initializing the actor. Level-9 setup independently populates the same
cell at `374fd`. All 256 indexed exact-cell references were freshly decoded:
each accesses four bytes, and their operand fields account for all 256 original
HIGHLOW references to that address. The explicit loader-zero gate checks the
outside-caller anchor, native storage and separately compiled owner on every run.
This establishes cell identity, without proving pointer lifetime or a permanent
alias between the two actor-pointer cells.

The callers retain live pointer reloads, narrowing and call order. `2d320` keeps
the original six-case table even though its RNG argument is four. Original code
independently establishes the unsigned guard, table base, six destinations and
frame constants. Helper entries are grounded in their own native bodies and
source evidence; the frame helper also passes fresh complete RAW equality.

Four actual negative controls reject a neighboring zero cell, a truncated owner,
the wrong helper and the wrong switch destination. Journaled promotion freshly
checks all 345 RAW/RELOCATED proofs in 89 compiler calls, plus 43 legacy linked
checks. Their existing 86 broader provenance drifts remain unchanged. The
remaining 1,634 supported diagnostic rows pass in 200 sources. Source and
acceptance-gate code were unchanged in this batch.

`15670` remains deferred: its `6ef94` source owner declares a two-byte prefix,
while native `15c74` and `15cbc` read four bytes then mask to sixteen bits. This
batch does not establish complete native storage ownership for that prefix.

Bindings and independent identities are frozen in
`evidence/collision_owner_bindings_2026_10_25.json` and
`evidence/collision_owner_matching_2026_10_25.json`. Counts, checks and report
hashes are in `evidence/collision_owner_progress_2026_10_25.json`.
Recovery.json remains the sole acceptance authority. Function equality does not
establish dependency closure, historical source identity or whole-game behavior.
