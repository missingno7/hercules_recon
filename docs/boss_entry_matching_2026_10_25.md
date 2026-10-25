# Initialized boss Entry owner and caller

ENG1 `25580` gains complete 64-byte RELOCATED acceptance. The existing function
source and flags are unchanged. The ledger now has 480 accepted native entries /
48,368 bytes: 137 RAW, 300 RELOCATED / 34,160 bytes, and 43 retained linked rows.
All 479 earlier accepted records remain identical.

The missing `g_entries_5a450` now has a real initialized provider in
`src/eng1/behavioral/boss_entry_data.c`. Its 36 sixteen-byte records comprise
35 usable entries and the complete all-ones terminator. The existing RAW-accepted
`attach_entries` reads the tag at offset `e`, advances by sixteen and stops at
`ffff`. Fresh compilation of the complete owner produces 576 unrelocated data
bytes, with one full occurrence in native file-backed storage at `5a450`.
No function operand is used to solve that content identity.

The source retains the unchanged shared `Entry` declaration. Its opaque prefix
is initialized with explicit little-endian data fields; the three DWORD values
are representable signed integral multiples of 65,536. Their wider gameplay
meaning and the WORD at offset `c` remain unclaimed. The native first sentinel
is row35 at `5a680`; all sixteen terminal bytes are included. There is one native
HIGHLOW reference to the table address, in this caller. No additional independent
address witness is claimed: the table binding rests on its complete source
payload, unique native occurrence and consumer-established record boundary.

`attach_entries` itself freshly reproduces its entire 48-byte RAW contribution.
Two native direct-call witnesses outside the selected caller corroborate its
entry. The existing reference gate checks the caller's one data address and one
direct function reference. Wrong data base, wrong payload hash and wrong helper
entry controls reject. A changed data value and a truncated terminal row also
fail the complete-owner audit. No acceptance policy changed.

Run the reproducible data audit and original/source composition with:

```text
python scripts/boss_entries_differential.py
```

All 240 isolated cases pass with the real helper and initialized source owner;
there are no modeled service bodies. Cases vary initial actor bytes, flag and
render bits, and five sentinel positions. Early sentinels mutate only private
emulated data. The comparison checks the complete guarded actor region, ordered
stores, every tag read, helper arguments, call order, stack frame and preserved
registers. The stored table pointer is checked against its actual native/source
owner, then normalized by that role for cross-image memory equality. No native
instruction or original file is patched, and no original OS module starts.
These cases establish no later rendering or complete gameplay behavior.

The new source-build unit resolves `_g_entries_5a450`. The genuine ENG1 graph
now has 735 unresolved symbols, down from736, with unchanged 962 mapped providers
and 1,039 external function symbols. All 244 previous units retain every parsed
section/relocation and non-FILE symbol/aux record; FILE metadata is separately
validated. Their 1,213 complete function contributions are unchanged. The new
245th unit defines only this initialized data object. There are no duplicates
or generated providers. Linking still fails with its expected unresolved-symbol
exit25; this is one dependency resolved, not a playable engine.

Journaled promotion passes all 439 strict proofs and 43 linked function checks.
The 86 existing broader linked provenance drifts remain. All 321 Python tests
and the remaining supported masked diagnostics pass. Evidence is frozen in
`evidence/boss_entry_{references,bindings,runtime,progress}_2026_10_25.json`.
Full matching, dependency closure and runtime completeness remain separate.
