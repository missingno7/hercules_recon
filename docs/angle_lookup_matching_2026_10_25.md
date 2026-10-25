# Initialized angle tables and searches, 2026-10-25

Three complete 128-byte contributions gained FUNCTION_MATCH_RELOCATED acceptance:
ENG1 `2bf30`, ENG3 `1f7b0`, and newly recovered ENG1 `2bfb0`. The first two keep
their existing source, flags, symbols and native entries. The ledger now holds
404 accepted functions / 38,560 bytes: 136 RAW, 225 RELOCATED and 43 retained
linked rows. All 401 previously accepted records remain identical. Promotion
also adds the newly strict tangent symbol to 68 diagnostic co-defined function
inventories; their source, byte identities and reference maps remain unchanged.

The falsifiable question was whether existing initialized source tables could
identify these references independently of the callers' implied addresses.
Fresh compilation of the unchanged `calibration/title/title_sine.c` supplies the
complete signed-WORD sine table (2,048 bytes) and DWORD tangent table (1,440
bytes). Each full unrelocated payload occurs exactly once in file-backed,
non-executable storage in each of TITLE, ENG1 and ENG3. ENG1 roots are `5a848`
and `5b048`; ENG3 roots are `38688` and `38e88`. These payload searches establish
the roots before instruction/reference comparison. Six wrong-hash and
shifted-base controls reject despite zero non-reference instruction differences.

The tangent search takes two unsigned WORD inputs, but its shifted numerator
and quotient are signed DWORDs. Inputs with the high WORD bit set therefore
retain historical negative-quotient behavior. Its zero-denominator result is
`0x5a0000`. The adjacent ENG1 search uses the descending sine-table half-period
at indices 256..767. A zero denominator returns zero first; a zero numerator
returns 255. Otherwise it preserves wrapped left-15 arithmetic, signed division,
exact hits and lower-bracket results. It returns a WORD phase.

Caching each signed table load in an `int` makes VC5 reuse the comparison flags
and removes 66 non-reference byte differences. Repeating the narrow expression
or changing only to C++ did not. The resulting ordinary C contribution matches
all 128 bytes after independently verified references, including padding.
No acceptance rule changed.

`scripts/angle_lookup_differential.py` reproduces the complete table audit and
3,522 isolated, unpatched original/source cases in pinned Unicorn 2.1.4. They
include edge inputs, seeded DWORDs, table subdivisions, signed quotient wrap,
and one agreeing `INT_MIN / -1` divide exception at the same function offset.
Normal returns preserve the cdecl frame, ESP and callee-saved registers. Execution
is confined to the selected contribution; writes are confined below its stack
frame. No OS module load, initializer or game startup occurs. These finite
fixtures do not establish whole-game behavior or historical source identity.

The actual ENG1 source graph now consumes `src/shared/angle_lookup.c` and the
existing initialized table owner. Reviewed aliases connect `eng3_1f7b0` to the
ENG1 tangent provider, `eng1_2bfb0` to the phase provider, and
`g_sine_table_5a848` to the complete sine table. The canonical tangent provider
has its own complete instruction/reference verification; the cross-module
`shared_2bf30` ledger row alone would not supply runtime source. The similarly
named `g_sine_table_70f34` remains unresolved because its identity is unproved.

The genuine ENG1 graph gains two mapped providers (957 / 1,231 diagnostic
spans), closes three unresolved names (747 remain), and has zero duplicate
providers or generated placeholders. It still does not link. All 363 strict
proofs and 43 legacy linked function checks pass; the 86 existing broader linked
provenance drifts remain unchanged. All 218 Python tests and 1,625 remaining
supported masked diagnostics in 199 sources pass. Matching, source closure and runtime
completeness remain distinct.

Evidence: `evidence/angle_lookup_recovery_2026_10_25.json`,
`evidence/angle_lookup_bindings_2026_10_25.json`,
`evidence/angle_lookup_runtime_2026_10_25.json`, and the final frozen progress
receipt `evidence/angle_lookup_progress_2026_10_25.json`.
