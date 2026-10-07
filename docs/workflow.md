# Reconstruction work and compact context

`recovery.json` remains the only recovery authority. The sequence is evidence,
private scratch, fresh compiler/oracle measurement, and the existing journaled
promotion. No model opinion or context receipt grants `FUNCTION_MATCH`.

Use **Luna xhigh** for normal reconstruction in a reasonably stable frame. Use
**Sol high** when the explanation itself is uncertain or resolving one shared
question can unlock several functions. Hand its falsifiable hypotheses back to
Luna. A failed candidate, function size or ugly diff is not an escalation rule.
There is no scheduler, difficulty score, retry ladder or routine Sol review.

Choose the largest *interpretable* common context. Label ownership as proven TU,
inferred region, or function task; convenience does not prove historical ownership.
Compile broadly once, freeze exact contributions, investigate shared causes once,
and shrink to local families when common representation is stable. Relocation-only
equals belong to the dependency/linkage problem, not an algebraic source search.

## Start and resume

`python scripts/region.py packet <region>` derives a small HOT packet from the
explicit index in `evidence/region_index.json`, selected WARM receipts, and current
`recovery.json`. `--more` adds reviewed target spans. COLD disassembly, verbose logs
and superseded variants stay in ignored scratch and are opened only for a specific
question. The index is a reading list, not task status or a second recovery ledger.

Packets identify source/context, unresolved functions, selected frontier, tried
families, blockers, re-entry evidence, next discriminator and ungated work. Changed
baseline/oracle/flags/symbol context invalidates old receipts. Canonical closure is
overlaid from recovery state each time; fully closed regions expose no frontier.
Workers own disjoint `work/<lane>` and `candidates/<lane>` paths. One integrator
publishes compact durable evidence and promotes source; workers never edit shared
status logs or accepted state.

## A bounded experiment

Write metadata with region/lane, family, prediction, falsifier, model/effort,
capability and, when available, elapsed time/build count. Unknown tokens/cost stay
null. Before a family, run:

```
python scripts/region.py check --spec SPEC --baseline BASE --meta META --prior RECEIPT
```

Published negative evidence is also checked. An exhausted/falsified/blocked family
requires new evidence, a new discriminator, or changed context. This is a guard
against accidental replay, not an automatic judge of a hypothesis's quality.

Compile with the existing pinned tools and run the relevant semantic harness.
Record the resulting report/object without another exploratory build:

```
python scripts/region.py record --spec SPEC --baseline BASE --source CANDIDATE --report REPORT --meta META --parent RECEIPT
```

This rechecks full raw contributions and source/object/oracle identities. It does
not prove compile freshness independently of the reported experiment, cache proof,
or replace fresh promotion. A receipt retains a baseline-bound source patch,
compact measurements, provenance and unknown economics explicitly. Resume with
`materialize --receipt RECEIPT --baseline BASE --output candidates/LANE/new.c`.
Stale source recipes fail rather than silently applying elsewhere.
Recipe source identities use UTF-8/LF text; the actual compiler-input byte hash is
recorded separately. Restored text is always freshly compiled when verifying it.

Output identities include complete function bytes and ordered relocation names,
types and offsets, plus non-code contributions for the measured-region identity.
They exclude object timestamps and paths. Local label renumbering may cause a
conservative false distinction; equality is never manufactured by masking operands.
Equivalent outputs share a representative; regressing raw-exact peers are excluded.
A small diagnostic Pareto display retains useful alternatives; it does not route
models, predict difficulty, or change the acceptance decision. Receipts and compact
failed-family conclusions survive; no destructive scratch garbage collector runs.

Keep successful sources and reproducible recipes, not every candidate spelling.
Measure progress in accepted bytes/functions and explained blockers, not variant
counts. See `docs/workflow_experiment.md` for the actual transfer and production run.
