# Constrained TITLE link pilot, 2026-10-23

The user authorized explicit layout constraints and documented compiler steering
provided logic is preserved. This first experiment changes only function order
through the unmodified pinned VC5 RTM linker's `/ORDER` option. It uses the same
source graph, flags, exports, libraries, and trace-free diagnostic profile. No
game source changed, no original code was supplied to the build, and no emitted
instructions were patched. Default behavioral builds are unchanged.

## Measured outcome

| Measurement | Before | Explicit function order |
| --- | ---: | ---: |
| Mapped game functions | 362 | 362 |
| Functions at original RVAs | 21 | 275 |
| Complete raw-equal linked functions | 13 | 25 |
| Raw-equal function bytes | 992 | 2,112 |
| Raw-equal functions also at original RVAs | 2 | 16 |
| Reference-only mismatches | 326 | 314 |
| Other code-byte mismatches | 20 | 20 |
| Function-size discrepancies | 3 | 3 |

Every section's contents, flags, size and relocation records are identical across
all 43 compiled units; all non-FILE COFF symbols are identical too. COFF timestamps
and FILE auxiliary records containing different private source paths are excluded
from that comparison, explicitly. This is not whole-object byte identity.

The narrow pilot checks `_title_04410`, `_title_04610`, and `_title_04710`:
96 + 80 + 80 = **256 bytes**, including padding, equal the original at the original
addresses. All eight direct REL32 references have zero addends and reach the same
named function entries in both the native ledger and the linked map. The checker
rejects missing/wrong callee identities, indirect transfers, nonzero addends,
unaccounted external branches, object-to-linked instruction changes, partial
decoding, or any byte difference. The recorded callee identities are consulted
independently of the pilot's inferred operand bindings.

This establishes the pilot functions' instruction and call-destination equality.
It does not establish the called functions' implementations or all behavior of
the DLL. Code that depends on numeric code addresses outside the pilot would need
separate review. No runtime integration test was run. All eight accepted TITLE
peers remain exact, and the global raw acceptance gate still has 134 functions /
8,336 bytes. `recovery.json` is unchanged by this experiment; the 25 observations
are not silently promoted.

## Reproduction

```text
python scripts/title_behavioral.py --matching-diagnostic --output work/LANE/base
python scripts/title_behavioral.py --matching-diagnostic --original-code-order --output work/LANE/ordered
python scripts/title_match_diagnostic.py work/LANE/ordered/report.json --linked
python scripts/title_order_check.py work/LANE/base/report.json work/LANE/ordered/report.json
python scripts/match.py verify --report work/LANE/strict.json
python -m unittest discover -s tests
```

The measured baseline is `work/title_match_progress_2026_10_23/title_no_trace/`;
the final constrained build is `work/constrained_title_2026_10_23/ordered_cdecl/`.
Both originate from fresh historical compilation. The preservation checker seals
the reports, objects, compiler inputs, images, maps, recovery and source/header
contexts; altered input rejects the result. The order file is also hash-checked.
The automated suite has 113 passing tests.

The first trial used COFF-decorated `_name` entries in the order file. LINK 5.00
adds an underscore itself, emitted LNK4037 warnings for `__name`, and ignored the
order. That trial did not improve placement. The corrected file strips exactly
one C cdecl underscore; unsupported naming conventions are rejected. The builder
now rejects linker warnings and verifies strictly increasing mapped addresses,
so successful exit alone cannot claim that ordering was applied.

## Next work

The new order produces 12 additional raw-equal functions / 1,120 bytes and makes
layout mistakes easier to isolate. Next review the remaining data/reference
ownership and the three size discrepancies. Extend reference checks only to
well-defined additional cases; keep data, imports, indirect calls, aliases and
callees' semantic correctness explicit. Do not use the user's conditional
permission to justify unknown logic changes or copied code.

Compact provenance and results are in
`evidence/constrained_title_2026_10_23.json`. Generated artifacts remain ignored.
