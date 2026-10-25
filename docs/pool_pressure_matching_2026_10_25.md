# Alternate-pool pressure helper, 2026-10-25

ENG1 `1b1d0` now has a real source provider in
`src/eng1/behavioral/alternate_pool_pressure.c` and complete relocated acceptance
for 96 bytes, including eight NOP padding bytes. The ledger contains 422 accepted
native entries / 41,328 bytes. All 421 earlier accepted records remain unchanged.

The routine captures the alternate pool pointer and zero-extended WORD primary
count. Its first scan returns if **any** primary slot has kind zero. If all slots
are occupied, the second scan releases the first slot whose zero-extended WORD
kind equals the full DWORD argument. It does not reserve a slot, allocate an
actor, or guarantee that release succeeds. All five observed callers push
`0x202f` and leave the result unused before their next cdecl call.

The initial pointer-stream draft occupied the same 96-byte span but retained
17 positional differences and misplaced absolute references. Type alignment,
the full actor type, explicit initialization, declaration order and a register
hint did not fix those differences. Reusing the scan cursor, reset to the
captured pool at the start of the second pass, makes the complete contribution
instruction-equal. Advancing the separately saved pool in the second pass
changes register allocation. No filler, volatile qualifier, object patch or
original-code dispatch was introduced.

Both scalar references use their existing actual same-name owners. The pool
pointer is the complete four-byte COMMON object in `actor_storage.c`, identified
independently by the allocation output slot at `1f15e`. The WORD limit is the
complete two-byte COMMON object in `frame_data.c`, independently populated at
`281dd`. Fresh owner builds, loader-zero extents and original HIGHLOW instruction
anchors are checked by the existing gate. The release entry is independently
identified from its own 304-byte body, existing source and an outside resetter
call. Its implementation remains unaccepted; accepting this caller does not
accept the release/resource chain. Three wrong-reference controls reject despite
unchanged non-reference instructions. No new alias mechanism was needed.

`scripts/pool_pressure_differential.py` passes 382 original/source cases in
Unicorn, with 329,844 kind reads and 49 release cases. It checks complete ordered
read traces, first-match call pointers, captured globals, synthetic bank/stack
guards and callee-saved registers. Cases include zero count with a null pointer,
free slots before/after matching slots, repeated kinds, upper DWORD argument
bits, release side effects, and a fully readable synthetic 65,535-slot bank.
The release service is modeled. These fixtures establish no native allocation
capacity or complete lifecycle behavior. No original OS module or host/game is
started. Incidental EAX is excluded from the void-effect comparison.

The preceding HIGHLOW census found this entry because an inline jump table had
desynchronized the section-wide database disassembly. `leaf.py packet` now
decodes from the requested entry and reports any undecoded tail. Inline data
still needs review; a packet cannot establish a complete function extent or
matching acceptance.

The fresh ENG1 source build has 962 mapped providers out of 1,231 diagnostic
spans, with the same 744 unresolved symbols, zero duplicate providers and no
generated placeholders. All 381 strict proofs, 43 legacy linked function checks,
245 tests and 1,612 supported masked rows pass. The existing 86 broader linked
provenance drifts remain. This is one new provider; link closure remains incomplete.

A subsequent private control expresses both allocator scans with direct global
cursor/bound/pool expressions. It reproduces all 336 bytes at `26f70` and all
320 bytes at `27b00`, where the existing captured-local source has compiler
differences. These diagnostics are neither integrated nor accepted. Their shared
TITLE ledger rows and actual ENG1 provider ownership need review before migration;
independent scalar anchors and first-fit/wrap comparisons remain necessary.

The [allocator/initializer follow-up](allocator_matching_2026_10_25.md) completes
that reference review, integrates both providers and accepts all 656 bytes. It
also recovers and RAW-accepts the 272-byte unconditional base initializer. This
document and its frozen receipt retain the earlier snapshot.

Evidence: `evidence/pool_pressure_recovery_2026_10_25.json`,
`evidence/pool_pressure_bindings_2026_10_25.json` and
`evidence/pool_pressure_progress_2026_10_25.json`. Recovery.json remains the sole
acceptance authority. Full matching is incomplete.
