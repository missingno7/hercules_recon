# Pool startup and shutdown callers, 2026-10-25

Three unchanged ENG1 functions gain complete relocated acceptance: shutdown at
`256a0`, normal startup sweep at `274e0`, and alternate startup sweep at `27fa0`,
64 bytes each. The ledger has 428 accepted native entries / 42,448 bytes:
137 RAW, 248 RELOCATED and 43 retained linked rows. Every earlier accepted record
is unchanged. No gameplay/storage source, header or verifier policy was edited.

Shutdown calls the normal, middle and alternate resetters in that order, then
restores the three alternate scan cursors from their current starts. Each startup
sweep reloads its WORD limit and pool pointer across initializer calls, with
stride `134` or `94`. Eight complete WORD objects in `frame_data.c` and two
pointer cells in `actor_storage.c` supply the actual owners. Configuration stores
outside all selected callers identify the WORD roots; allocation output slots
identify the pointers. The newly reviewed middle cells `713aa` and `713b8` are
both populated from selected row WORD+`a`. The scoped HIGHLOW census classifies
57 directly overlapping WORD accesses, retaining adjacent non-overlapping
operands separately. Computed aliases and native capacities remain unproved.

The five callee entries already have independent complete function proofs.
Fifteen wrong-target controls reject with unchanged non-reference instructions.
Accepted callee identity does not transfer complete lifecycle or dependency
closure to these callers.

The startup source was already a real source-build provider. Its three shared
diagnostics migrate to that unchanged unit, with complete old rows retained in
the reference review. All 38 old-unit diagnostic peers were freshly checked;
TITLE/ENG3 still record the symbols, so its complete function inventory is
unchanged. The normal and alternate rows then pass the existing strict gate.
The middle startup row stays CODEGEN_SIMILAR: its indexed pointer-table ownership
is unclosed. No indexed BSS or pointer-alias support was added.

`scripts/pool_callers_differential.py` passes 168 guarded original/source cases:
100 composed shutdown cases, 20 normal sweeps, 28 middle sweeps and 20 alternate
sweeps. Shutdown executes all three real reset loops; lifecycle services are
modeled. Callbacks shrink/grow limits, switch pool banks and update all cursor
starts, including WORD wrap. The current actor still finishes if a callback
shrinks the count to zero. Cursor restoration occurs after all resetters return
and uses the final starts. Complete call traces, bank contents, globals, stack
guards and callee-saved registers agree. Middle cases also compare ordered `-1`
stores through four synthetic group pointers after its initializer loop. Those
fixtures prove neither native table capacity nor matching acceptance of that
middle routine. No original OS module or host/game starts.

All 387 strict proofs, 43 linked function checks, 245 tests and 1,607 supported
masked rows pass. The source graph retains 962 mapped providers and the same
744 unresolved symbols, with no duplicate or generated providers. Existing
86 broader linked provenance drifts remain. Full matching is incomplete.

A private follow-up tests pointer-name aliases without changing the verifier.
Literal caller renaming changes complete function contributions in all three
tested caller units, so merely allowing size four would fail the existing proof.
The data-only control is promising: renaming the complete COMMON owner in
`actor_storage.c` or `actor_control_cells.c` preserves every section and external
inventory after the explicit name map. Neither owner unit contains functions.
This is diagnostic evidence only. A distinct owner-renaming proof would need
compiler-checked pointer ownership, exact original/adapted COMMON objects, caller
undefined-DATA checks, immutable input/object guards, adversarial tests and all
existing native reference checks. No pointer-alias capability is enabled here.

Evidence: `evidence/pool_callers_references_2026_10_25.json`,
`evidence/pool_callers_bindings_2026_10_25.json` and
`evidence/pool_callers_progress_2026_10_25.json`. Recovery.json remains the sole
authority for accepted function matching.
