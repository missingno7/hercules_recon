# Attachment Entry family and level initializer

Four ENG1 functions gain RELOCATED acceptance: `3a730`, `3a8a0`, `3a910`
(64 bytes each) and `33e10` (112 bytes). The ledger now contains 484 accepted
native entries / 48,672 bytes: 137 RAW, 304 RELOCATED / 34,464 bytes, and
43 retained linked rows. All 480 previously accepted records remain identical.

`src/eng1/behavioral/attachment_entry_data.c` supplies four missing initialized
owners, preserving the callers' existing unsigned-char array declarations:

| Native root | Complete bytes | Usable records | Tag before terminator |
| --- | ---: | ---: | ---: |
| `64510` | 464 | 28 | 0 |
| `646e0` | 240 | 14 | 0 |
| `647d0` | 144 | 8 | 0 |
| `5e688` | 272 | 16 | 5 |

Each includes its full sixteen-byte all-ones sentinel. The RAW `attach_entries`
consumer establishes the sixteen-byte stride and tag WORD at offset `e`.
Fresh compilation reproduces all 1,120 source bytes. Each complete payload has
one native file-backed occurrence; wrong-value and truncated-owner controls
reject for all four objects. The signed integral 16.16 values are representable
without overflow. Coordinate meanings, WORD+c meanings and later rendering
remain outside this data recovery. No caller operand is used to solve the full
payload identity.

The first three callers keep their source, flags and two references unchanged.
Their real RAW helper and initialized owners pass 720 cases, with no service
models: varying actor bytes, flag/render bits and five first-sentinel positions
per table. Every actual tag read is checked against its own complete owner and
the exact expected prefix. Ordered stores, complete guarded actor memory, call
arguments/order, raw pointers, stack and preserved registers agree.

The fourth caller's old `g_level_word_pool_554f0` name addresses the interior of
the existing complete 192-byte `g_eng1_config_word_pool_55478` owner. Its full
source payload is independently located at `55478`; the required root is
WORD+60 / byte+120. A new real provider,
`src/eng1/behavioral/entry_level_initializer.c`, uses that explicit interior
reference and reproduces all 112 bytes through the unchanged reference gate.
It defines no duplicate WORD storage. The data bytes, two RAW helper entries
and WORD-interpreter entry are independently reviewed; their five wrong-target
controls reject. The other three callers' nine wrong-binding controls also reject.

The old calibration source remains byte-identical, preserving its thirteen
accepted function records. Only `eng1_33e10` becomes a declaration in the derived
runtime source plan. Its previous complete ledger record is retained in the
review, and all 31 remaining old-unit masked rows pass before migration. The new
function remains a diagnostic until the journaled relocated promotion succeeds.
The earlier attachment reference review is an immutable first-phase snapshot
that predates this fourth adoption.

Another 480 composed cases exercise `33e10` with real RAW reset/attachment
helpers and an explicit WORD-interpreter model. The model clobbers volatile
registers and optionally changes count, parameter, flags, image pointer and
category. The caller retains helper effects, attaches the table, reloads the
resulting count into field 98, then sets parameter 6. The model proves none of
the WORD interpreter's command semantics or dependency closure. Its independently
established entry identity is sufficient only for this caller reference proof.

Reproduce the two bounded compositions with:

```text
python scripts/attachment_entries_differential.py
python scripts/entry_level_initializer_differential.py
```

No original OS module starts. Early sentinels modify only isolated data; no
instruction or original file is patched. Raw pointer values are checked before
normalizing owner roles for cross-image memory equality.

The final genuine graph has 247 units, 962 mapped providers, 1,039 external
function symbols and 730 unresolved dependencies, down from 735. Four initialized
owners and the old WORD-root alias are resolved. All 244 unaffected prior units
retain their complete parsed sections/relocations and non-FILE symbol/aux records.
The changed derived unit retains fifty other complete functions and all its
noncode storage except the retired initializer's FPO record. One compiler-private
static WORD changes its generated suffix from 608 to 607: the audit separately
checks its identical source function, complete two-byte BSS owner, identical
instruction bytes and both DIR32 uses before mapping that name. This is a private
source-graph comparison, with no scalar/BSS gate exception or object patch.
No initialized game data is retired. There are no duplicates or generated
providers; the expected unresolved-symbol link exit 25 remains.

All 443 strict proofs, 43 linked checks, 321 Python tests and 1,551 supported
masked diagnostics pass. The existing 86 broader linked provenance drifts remain.
Evidence is frozen in `evidence/attachment_entry_*_2026_10_25.json` and
`evidence/entry_level_initializer_*_2026_10_25.json`; the final census and audit
hashes are in `evidence/attachment_entry_progress_2026_10_25.json`.
Full matching, complete gameplay and full source-link closure remain incomplete.
