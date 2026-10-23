# TITLE matching progress, 2026-10-23

The two proposed matches are accepted: `title.016e0` and `title.01dd0`, each a
complete 16-byte relocation-free contribution including padding. They remain in
`calibration/title/r01000.c` and `calibration/title/r01a30.c`; neither source moved
or changed. `recovery.json` now records **134 FUNCTION_MATCH functions / 8,336
bytes**, including eight TITLE functions / 384 bytes. Only these two ledger rows
changed relative to the start of this step. The preceding scheduler adoption and
other user changes were preserved.

The new `promote.py --existing-unit` path records the full COFF inventory and
recursive source/header context, protects all eight functions in the two units,
and changes selected statuses through the existing lock/journal mechanism. Data
inventory preservation is not a claim that the data matches the original. Fresh
verification passes all 134 accepted rows in 32 historical compiler calls. The
masked verifier passes 447 supported rows in 73 sources, including protection of
raw peers in mixed units. All 105 automated tests pass.

## Complete linked diagnostic

The ordinary source-only diagnostic build closes all 362 mapped game entries
from 43 sources, without unresolved names, duplicate providers or forced linking.
The new raw report uses the full emitted function contribution and full original
span, never a target-sized prefix. It validates object/report/map/image hashes,
checks that non-relocation object bytes survive linking, and compares complete
raw sections and headers without rewriting operands. Unrecorded target extents
remain provisional. Source ordering is a diagnostic hypothesis, not recovered
historical layout.

| Complete linked contribution | Functions | Target bytes |
| --- | ---: | ---: |
| Raw equal | 13 | 992 |
| Differences only in reference operands | 326 | 80,416 |
| Other code-byte differences | 20 | 26,208 |
| Size differs | 3 | 9,040 |

Only two of the thirteen raw-equal linked functions are at their original RVAs.
Five are additional diagnostic leads: `_title_04410`, `_title_04610`,
`_title_04710`, `_title_08cd0`, `_title_090f0`. They are not promoted by this report.
Call-bearing functions require separate acceptance and reference/placement review.
All eight previously accepted TITLE contributions remain exact.

Removing our optional entry tracing and its support provider from this diagnostic
profile reduces the DLL from 220,160 to **181,760 bytes** (original: 180,736).
This removes instrumentation-driven library dependencies; it does not rewrite
game logic. The default behavioral build still enables tracing. The entry
function shrinks from 320 to the native span of 128 bytes, but still has 34
non-reference byte differences and is not a match.

| Raw section size | Before trace-provider removal | After | Original |
| --- | ---: | ---: | ---: |
| .text | 163,328 | 133,632 | 130,560 |
| .data | 32,768 | 29,184 | 29,184 |
| .rdata | 3,072 | 1,024 | 1,024 |
| .idata | 2,048 | 1,536 | 1,536 |
| .reloc | 17,920 | 15,360 | 17,408 |

Equal sizes do not imply equal contents. The final image differs at 157,414 file
offsets. It has 50 imports versus 46; the additional imports are `FatalAppExitA`,
`GetCurrentThread`, `MultiByteToWideChar`, and `UnhandledExceptionFilter`. Library
contributions should be identified, not recreated as game functions.

## Next discriminating work

1. Recover real data ownership and placement. Among comparable functions there
   are 4,159 unequal reference operands targeting native .data and 1,656 targeting
   .text. Do not change source spelling in these 326 functions to compensate for
   the missing layout. Compare complete initialized storage, BSS and library
   contributions, preserving genuine aliases and structure member offsets.
2. Review four existing binding-view changes independently: `title.03050`,
   `title.066f0`, `title.19570`, `title.195f0`. These are interior globals replaced
   by base-object views in the complete graph. Masked code bytes agree, but the
   recorded symbol maps differ; no automatic normalization was added.
3. Diagnose the complete size discrepancies: `_title_054f0` 1,376 vs 1,408,
   `_title_0a450` 7,440 vs 7,408, `_set_decimal_digits` 240 vs 224. The first two
   extents remain provisional inventory spans. Freeze each unit's peers and state
   a prediction/falsifier before changing source. The separate `186e0` residue
   remains a bounded future code-generation experiment.

## Reproduction and provenance

Use the pinned VC5 RTM toolchain in `toolchains/manifest.json`. On this Windows
host the usable interpreter is
`C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe`.
Historical compiler launches require approved execution outside the sandbox after
the previously diagnosed WinError 623. No tools were modified or installed.

```text
python scripts/promote.py title.016e0 title.01dd0 --existing-unit --report work/LANE/promotion.json
python scripts/match.py verify --report work/LANE/strict.json
python scripts/similar.py verify
python scripts/title_behavioral.py --matching-diagnostic --output work/LANE/title
python scripts/title_match_diagnostic.py work/LANE/title/report.json --linked
python -m unittest discover -s tests
```

The promotion command is a one-time transition and rejects already accepted IDs.
Use strict verification to reproduce their acceptance after this checkpoint.
Private receipts live under `work/title_match_progress_2026_10_23/`; the final
build is `title_no_trace/`. The earlier `title/` report is a frozen before-change
measurement and its build context no longer matches the current tracing config.
Compact hashes, results and predictions are in
`evidence/title_matching_progress_2026_10_23.json`. Generated objects, DLLs, maps
and detailed reports stay ignored. No assets were modified; the earlier audit's
unregistered PSX image remains a separate provenance issue.
