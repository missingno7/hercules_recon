# Spatial neighbors and their callers, 2026-10-25

Ten functions gained acceptance, covering 1,088 complete contribution bytes:

| ENG1 entry | Bytes | Proof | Role |
| --- | ---: | --- | --- |
| `2a950` | 144 | RELOCATED | Copy extents into actor bounds |
| `2a9e0` | 80 | RELOCATED | Horizontal screen predicate |
| `2aab0` | 128 | RELOCATED | Flag-gated screen predicate |
| `2ad70` | 224 | RAW | Actor-target vector estimator |
| `2ae50` | 80 | RELOCATED | Projection and screen flags |
| `34350` | 80 | RELOCATED | Actor update and screen removal |
| `360f0` | 48 | RELOCATED | Screen predicate and actor release |
| `d350` | 48 | RELOCATED | Script update and screen removal |
| `10b10` | 128 | RELOCATED | Target-distance transition at eight |
| `10dc0` | 128 | RELOCATED | Target-distance transition at thirty-two |

The ledger now contains 400 accepted functions at 400 distinct native entries,
covering 37,216 bytes: 136 RAW, 221 RELOCATED and 43 retained linked rows. All
390 earlier accepted records remain identical. Two linked rows retain RAW proofs
and are counted once.

Five missing bodies now have real source in
`src/eng1/behavioral/spatial_neighbor_helpers.cpp` and
`src/eng1/behavioral/actor_target_relative.c`. The estimator reuses the established
`shared/relative.c` algorithm with the second position at actor+d8/dc/e0. It keeps
DWORD subtraction wrap, signed high-WORD extraction, signed-short absolute-value
overflow, magnitude writes before sorting, direction bits and both original
distance estimates. Its complete 224-byte relocation-free contribution passes
the unchanged RAW gate in a fully inventoried one-function source unit.

The bounds setter passes the low WORD of mode to the real extent producer and
returns zero on producer failure or zero x/y extents. A shared failure block
reduced its residue from 69 to fourteen bytes. C/C++, array/named fields and the
full actor declaration context retained that residue. Natural per-axis copy
statements reproduce the exact native load/store schedule. The screen-flag
helper's Y-before-X zero assignments close its measured two-byte scheduling
difference. Both changes preserve the recovered logic; no assembly or output
patching is used.

The screen helpers reuse the independently audited two-byte pipeline WORD owner
at `71376`. The extent producer's own earlier native body/ABI evidence supplies
its entry. The projection helper has an existing accepted proof that was freshly
checked. Five existing callers then pass complete reference-aware comparison:
three screen consumers and two interpreter/movement/distance transition chains.
The movement result is consumed as AL, and removal/release remain distinct entries.
Transition helpers with no outside caller witness are independently grounded by
their own accepted complete proofs.

There are 24 checked references: 21 direct calls and three WORD-owner uses.
Actual negative controls reject the wrong extent entry, neighboring zero cell,
wrong projection entry, substituting removal for release, and changing the
distance coefficient from eleven to ten. The wrong coefficient still produces
224 bytes but differs at 26 positions and fails RAW comparison.

Final promotion freshly checks all 359 RAW/RELOCATED proofs in 94 compiler calls,
plus all 43 retained linked function checks. The existing 86 broader linked
provenance drifts remain. All 218 Python tests and 1,627 supported diagnostic rows
in 199 sources pass. No previously accepted source or verifier/gate code changed.

The genuine ENG1 source graph now has 953 mapped providers out of 1,231 diagnostic
spans, with zero duplicates or generated placeholders. One obsolete placeholder
was removed. Linking remains incomplete with 751 unresolved symbols: 441 function
and 310 data/untyped names, down from 757. Extent-producer dependencies, full data
capacities and whole-game behavior remain unproved. This batch uses deterministic
complete-contribution and independent-reference proofs; it adds no runtime-case
claim or original host/game execution.

Evidence and bindings are in `evidence/spatial_neighbors_recovery_2026_10_25.json`,
`evidence/spatial_neighbors_bindings_2026_10_25.json`,
`evidence/spatial_neighbor_callers_2026_10_25.json` and
`evidence/spatial_neighbor_caller_bindings_2026_10_25.json`. The frozen final receipt
is `evidence/spatial_neighbors_progress_2026_10_25.json`. Recovery.json alone grants acceptance.
