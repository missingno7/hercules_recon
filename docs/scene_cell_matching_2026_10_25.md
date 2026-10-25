# Support/scene pointer cells, 2026-10-25

ENG1 `295a0` and its ENG3 `1e440` twin gained complete relocated acceptance,
128 bytes each. The ledger now has 417 accepted native entries / 40,816 bytes:
136 RAW rows, 238 RELOCATED rows and 43 retained linked rows. All 415 earlier
accepted records remain identical. No canonical function or storage source changes.

The question was whether the already declared `g_scene_anchor` is a complete
scalar owner independently identifiable in each original image. Fresh compilation
of the unchanged `src/eng1/behavioral/actor_storage.c` emits exactly one four-byte
COMMON definition of that name. Both caller contributions use the same name;
no source alias or alternate owner spelling is introduced.

The native support-query writes at ENG1 `29879` and ENG3 `1e6d9` independently
identify the corresponding cells at `6fed8` and `3fbc0`. Each anchor is outside
both selected insertion functions, is one complete MOV with one original HIGHLOW
field, and belongs to the support search that clears/stores an actor pointer.
Each complete cell lies inside one writable, non-executable loader-zero tail,
outside PE directories and fixups, without unreviewed TLS/load-config effects.

All nine direct original HIGHLOW references to the ENG1 cell and six to the ENG3
cell decode as DWORD accesses at the root. No interior-byte reference is found
in that inventory. This is not an exhaustive claim about computed-address
aliases. The source declaration supplies the complete four-byte object shape;
the native support/query/insertion roles supply its identity. Zero contents
alone supply neither.

The same declaration represents separate module-qualified cells for matching.
The ENG1 source file's pathname does not create an ENG3 runtime provider or
coalesce those native cells in a merged runtime. Cross-module matching, source
integration and whole-game state remain separate dimensions.

`scripts/scene_insert_differential.py` checks 896 synthetic cases per engine,
1,792 original/source comparisons. It covers `80000` rejection, `40000`
traversal, zero/default/explicit/self anchors and acyclic chains of up to three
other nodes. Full return pointers, complete fixture memory, the unchanged anchor
slot, cdecl frames, ESP and callee-saved registers agree. Return values retain
the originally selected anchor even when insertion walks farther along a chain.
The fixture's extracted function must itself compile instruction-equal to both
native entries before linking. Execution stays within each 128-byte body, with
writes confined to fixture data and the stack. No OS module load or game startup.

Six wrong-neighbor, oversized-owner and owner-alias controls reject through the
existing verifier. Both complete instruction/reference proofs use freshly
compiled same-name owners; no gate change, indexed BSS support, alias relaxation
or guessed list capacity is involved.

The same audit independently confirms the existing middle/alternate pool pointer
slots: COMMON objects of four bytes at ENG1 `73494/734e0`, with bootstrap output
address witnesses at `1f139/1f15e`. These facts do not accept their resetters:
the limit references still use different source spellings from their current
WORD owners, and function-entry bindings remain separate. No allocated pool
capacity or meaning for the additional 308-byte allocation tail is inferred.

All 376 strict proofs, 43 legacy linked function checks, 218 Python tests and
1,616 remaining supported masked diagnostics in 198 sources pass. The 86
existing broader linked provenance drifts remain unchanged. Source-graph inputs
remain unchanged from the last 961-provider / 744-unresolved-symbol ENG1 build;
no new closure claim is made.

Evidence: `evidence/scene_cell_references_2026_10_25.json`,
`evidence/scene_cell_bindings_2026_10_25.json` and the frozen final receipt
`evidence/scene_cell_progress_2026_10_25.json`. Recovery.json alone grants matching
acceptance. Source identity, scene-state lifetime and whole-game completeness
remain unproved.
