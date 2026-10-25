# Actor allocator and base-initializer recovery, 2026-10-25

The normal allocator at `26f70` (336 bytes), alternate allocator at `27b00`
(320 bytes), and unconditional base initializer at `27fe0` (272 bytes) now have
real ENG1 providers and accepted function proofs. This adds 928 bytes, bringing
the ledger to 425 native entries / 42,256 bytes: 137 RAW rows, 245 RELOCATED rows
and 43 retained linked rows. All 422 earlier accepted records remain unchanged.

The allocators use sparse views containing only the observed kind offset `2e`
and strides `134`/`94`. Bit 15 selects the primary partition. Each searches from
its current cursor to its upper bound, then from the wrap start to the original
cursor. Success stores the found index and returns the slot. It does not advance
past that index or reserve the slot. Direct global expressions in the scan and
condition reproduce the complete native contributions; the existing captured-local
source differs. Compact views and actual canonical scalar names retain equality.

Ten complete WORD objects in `frame_data.c` and two complete pointer cells in
`actor_storage.c` supply the actual owners. Independent stores in the 416-byte
configuration routine at `280f0` identify the counter roots and row roles;
allocation output-slot pushes identify the pointer cells. The HIGHLOW census
classifies 76 directly overlapping WORD accesses, accounting separately for
adjacent non-overlapping operands. Fresh owner builds and the existing native
zero-tail/anchor/instruction/reference gate check every binding. Twelve wrong-root
controls reject with unchanged non-reference bytes. No alias support was added.

`scripts/actor_pool_allocators_differential.py` passes 1,922 guarded cases, 961
per allocator, with 396,071 ordered kind reads. It exhausts four-slot occupancy
patterns and cursor positions, tests wrap order, empty and non-monotonic bounds,
high argument bits, occupied WORD values, repeated calls and readable synthetic
65,535-slot banks. Return pointers, exact read order, cursor writes, all other
globals, complete banks, stack guards and callee-saved registers agree. Those
banks establish no native allocation capacity.

The two old ENG1 shared diagnostics were replaced with real ENG1 rows through
the existing diagnostic writer. Their complete old records remain in the review.
All 40 old-unit diagnostic peers were freshly checked; TITLE/ENG3 still record
both symbols, so the old unit's complete function inventory is unchanged. The
historical source files and their headers were not edited. Existing build
omissions select the new providers and retire redundant alternatives. Diagnostic
migration grants no acceptance; the unchanged relocated promotion gate then
recompiled and protected every accepted proof.

For the base initializer, the earlier source wrote the correct final record,
but its unsigned `+32` field and independent assignments compiled to 209 different
positional bytes. A signed short index assigned `-1` reduces this to 74; changing
only the literal has no effect. Seven native setup-interpreter loads use
`MOVSX WORD +32`, supporting a signed view without proving the original typedef.
The existing recovered TITLE initializer has that view and grouped zero
assignments. Reusing its source logic with the native ENG1 unconditional entry
yields complete RAW equality, including eight padding NOPs, with no relocations.
There is no object patch, filler or binary dispatch.

`scripts/actor_base_init_differential.py` adds 198 guarded cases comparing the
entire prefix and all 58 ordered stores per call. Eighteen bytes remain untouched:
`1a..1b` and `78..87`. Patterned/random inputs, repeated initialization, surrounding
guards and physical cdecl state agree. The RAW gate uses a new, fully inventoried
single-function unit; mixed-unit restrictions remain unchanged.

The source build initially lost the remaining normal initializer when its last
recorded sibling moved out of the historical source. That source is now selected
explicitly, with the four replaced bodies omitted. The final graph retains
962 mapped providers, the same 744 unresolved symbols, and zero duplicate or
generated providers; this is a build-dependency correction, not a new match.

All 384 strict proofs, 43 legacy linked function checks, 245 tests and 1,610
supported masked rows pass. The existing 86 broader linked provenance drifts
remain. Final graph objects also reproduce both allocator contributions and
preserve all 78 RAW functions selected in that source build.

All five indexed actor-pool entries now have accepted function proofs. This does
not validate every separate runtime provider or factory/resource chain: the
shared normal initializer's historical proof remains distinct from its runtime
provider. Native capacities and complete actor lifecycle behavior remain unproved.
No original OS module or host/game was started.

Evidence is in `evidence/allocator_references_2026_10_25.json`,
`evidence/allocator_bindings_2026_10_25.json`,
`evidence/actor_base_init_recovery_2026_10_25.json` and the final
`evidence/allocator_progress_2026_10_25.json`. Recovery.json alone grants matching
acceptance. Full matching remains incomplete.
