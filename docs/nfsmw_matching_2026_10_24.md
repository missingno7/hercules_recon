# Matching with bounded objdiff diagnostics, 2026-10-24

Ten existing ENG1 functions are accepted through the unchanged relocated gate:
`107c0`, `11290`, `11d30`, `36030`, `37fe0`, `384c0`, `38d10`, `38de0`,
`38ea0`, and `396e0`. Their complete contributions total 1,280 bytes with
55 direct-call references. Canonical game source was not changed.

Each helper's identity comes from its own earlier independently curated record,
followed by a fresh compilation and complete comparison at that helper's entry.
The caller's implied relocation destination did not choose the binding. The
`random_limit` name is explicitly reviewed as an alias of the host RNG bridge at
ENG1 `2c460`, whose copied slot, host destination, and unsigned return/argument ABI
are independently documented. It is not an original symbol-name claim.

Review preserved call order, separate RNG draws, and the discarded first draw in
`384c0`. No BSS, data-table, alias-addend, or new reference support was introduced.
A negative control bound `107c0` to the neighboring raw-RNG helper: the existing
verifier rejected both wrong call destinations despite equal non-reference bytes.

The journaled promotion freshly preserved all 294 prior RAW/RELOCATED proofs,
including the retained raw proofs on two legacy linked rows. It also rechecked
the 43 legacy linked matches with zero function failures. Their 86 broader
provenance drifts were already recorded in `evidence/realignment.json`; this
batch does not resolve that older source/image provenance limitation.

Current accepted state is 135 RAW rows, 167 RELOCATED rows, and 43 retained linked
rows: 345 unique rows / 27,792 bytes. All earlier accepted ledger records remain
unchanged. The existing promotion path also refreshed the co-defined-function
lists of 83 remaining diagnostic rows in the two caller units.
The remaining 1,666 supported masked rows pass in 201 sources, and all 183 Python
tests pass. Compact results and detailed-report hashes are recorded in
`evidence/nfsmw_matching_2026_10_24.json`.

## Process change

`scripts/objdiff_diagnostic.py` uses the optional unmodified, hash-pinned objdiff
3.7.0 CLI. `leaf.py residual --baseline-object` adds its compact previous/current
candidate comparison to the original-backed report after one fresh compile.
Reference differences are always enabled. Complete COFF extents, inferred body
sizes, byte differences, and ordered relocation equality remain separate fields.
Instruction insertions/deletions retain explicit alignment gaps and omission
counts. A 100% score, equal candidate bytes, or exit code zero grants no acceptance.

The adapter was exercised on a real one-byte branch change and a larger loop-form
change. Tests cover misleading 100% scores, bounded output, absent/ambiguous
symbols, and objdiff's instruction-less alignment rows. The historical compiler,
`match.py`, `masked.py`, and `promote.py` are unchanged.

## Deferred source finding

The host input initializer has one non-reference difference: the original at
RVA `7c1d` uses signed `JL`, while the current pointer comparison emits `JB`.
An ordinary signed counted-loop hypothesis changes 43 residual bytes and is
rejected. Explicit signed 32-bit address comparison changes only that branch and
removes every non-reference difference in the complete 160-byte contribution.
The bounded input differential passes all 15 scenarios / 350 API records,
including event order, ABI records, full initialized state, and write extents.

The exact differential-tested source is preserved as ignored
`candidates/nfsmw_matching_2026_10_24/host_input_signed_verified.c`; its hash equals
the differential runner's recorded compiler input.
The existing independent-evidence manifest pins the canonical input source for
earlier relocated proofs, so its temporary source/diagnostic staging was restored.
No unresolved storage identity was accepted. The tested correction is not a new
matching claim or an executed host-game result.

Automatic approval review rejected broadening mixed-unit promotion's peer/proof
handling. That patch was not applied. Matching continued through the existing
direct-reference acceptance path without changing the rejected gate.

## Reproduction

Use the configured Python and pinned VC5 tools; native compiler launches require
the already established execution outside the Windows sandbox.

```text
python scripts/match.py references --bindings evidence/eng1_direct_call_bindings_2026_10_24.json
python scripts/match.py verify --report work/check/strict.json
python scripts/similar.py verify
python -m unittest discover -s tests -q
```

Promotion is a completed one-time transition, not a repeatable verification
command. Compact independent identities and explicit uses are in
`evidence/eng1_direct_call_matching_2026_10_24.json` and
`evidence/eng1_direct_call_bindings_2026_10_24.json`. Detailed compilation,
candidate, differential, negative-control, and promotion reports stay under
ignored `work/nfsmw_matching_2026_10_24/`.
