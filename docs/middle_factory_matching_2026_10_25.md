# Middle allocator and factory matching, 2026-10-25

ENG1's middle allocator `275d0` (336 bytes) and coordinate factory `277e0`
(128 bytes) now have real, instruction-equal providers in
`src/eng1/behavioral/middle_actor_factory.cpp`. The source reuses the existing
TITLE twin's direct-root scan and assignment order, with actual ENG1 scalar
owners and a single sparse `0xec` actor view. Full-DWORD and WORD allocator
parameter controls both preserve complete equality; the WORD contract agrees
with existing consumers. This establishes no historical declarations or TU.

The ledger now records 479 distinct accepted entries / 48,304 bytes: 137 RAW,
299 RELOCATED / 34,096 bytes, and 43 retained linked rows. All 477 previous
accepted records remain identical. No source dependency is newly closed.

## Independent identities and behavior

Five existing WORD cells supply primary cursor/limit/split and alternate
cursor/wrap start (`713ac`, `71432`, `713a0`, `713b8`, `713aa`). The existing
four-byte pool pointer at `73494` and DWORD descriptor root at `71390` supply
the other owners. Fresh COMMON checks cover every complete scalar object.
Independent selected-row stores identify the configuration cells; the native
allocation output-slot PUSH at `1f139` identifies the pool pointer. The original
entry and zero-tail/fixup checks remain mandatory. A direct HIGHLOW census
classifies 58 scalar accesses and the one address-taking output argument; it
does not exhaust computed aliases or prove pool/descriptor capacities.

The initializer has an earlier accepted proof, freshly checked here. Native
call witnesses outside both new functions corroborate the allocator and
initializer entries. The allocator itself passes its complete instruction and
independent-reference gate alongside its factory caller. Nine deliberately
wrong reference bindings reject. No alias adapter or verifier change is needed.

`python scripts/middle_factory_differential.py` passes 1,111 guarded native/source
cases: 961 allocator cases and 150 factory cases, with 1,411 top-level calls and
198,898 ordered kind reads. The real middle and base initializers execute.
Allocator coverage includes exhaustive four-slot occupancy/cursor combinations,
wrap and empty/non-monotonic bounds, high physical argument bits, repeated calls
and readable 65,535-slot stress banks. These are synthetic capacities.

Successful allocation saves the found cursor without reserving the slot.
Repeated allocation can return it again. Creation initializes the slot and
stores `kind|4000`, so repeated creation consumes available slots and eventually
fails within that partition. Factory cases compare all ordered writes, paired
coordinates, descriptor reads, returns, surrounding storage and cdecl state.
Four descriptor/actor alias scenarios expose reads after coordinate stores;
36 successful invocations deliberately change the descriptor root between
category and image stores. These are explicit fixture states, not a claim that
real helpers change that root. No original OS module or game starts.

## Source integration and protection

The earlier runtime providers emit 352 allocator bytes and 176 factory bytes,
with 276/135 positional non-reference diagnostic differences. The replacements
emit the native 336/128 complete contributions and pass all reference checks.
The two unaccepted shared diagnostic rows migrate with their original records
retained in the review. All 33 old-unit diagnostic peers and its complete
inventory are checked freshly; the historical unit and headers stay unchanged.

Existing source-plan omissions retire only the two replaced bodies in
`actor_pool_reset_repaired.c` and `spawn_services.c`. Those canonical files stay
unchanged, and all six remaining complete function contributions are preserved.
Noncode sections also remain equal except the two retired bodies' FPO records
and their two guard-site strings / 78 bytes. The other 241 source objects retain
all parsed sections/relocations and non-FILE symbol inventories, with FILE
metadata validated separately. There is one provider per function and no new
unresolved symbol. Both new bodies also match under the integration flags.

All 438 strict proofs, 43 linked function checks and 321 Python tests pass.
The remaining supported masked diagnostics pass too. The existing 86 broader
linked provenance drifts remain. The graph still has 962 mapped providers /
1,231 diagnostic spans, 1,039 external function definitions, 736 unresolved
symbols and zero duplicates/placeholders. It does not establish module closure
or a complete playable game. The indexed five-entry actor-pool packet does not
include this allocator/factory pair; its earlier closure was a narrower claim.

Frozen references, runtime evidence and bindings are
`evidence/middle_factory_{references,runtime,bindings}_2026_10_25.json`.
Current counts and verification hashes are in
`evidence/middle_factory_progress_2026_10_25.json`.
