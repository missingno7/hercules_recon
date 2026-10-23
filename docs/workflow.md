# Everyday reconstruction workflow

Start with the [project direction](project_direction.md), current recovery state,
and the focused source/evidence for the function or dependency family. Work from
a falsifiable question; keep experiments under ignored candidates/ or work/.
Generated packets and reports are disposable and never grant acceptance.

## Inspect and compare a candidate

When the function map is indexed, make a compact context packet:

    python scripts/region.py packet <region>

Use --more only when shared context is needed. For a bounded source hypothesis,
compile against the original function with the current raw candidate command:

    python scripts/match.py candidate <id> --source candidates/<candidate>.c

This performs a fresh pinned-toolchain build. Inspect the complete contribution
and every mismatch; a prebuilt object is useful for diagnosis but cannot promote
recovery state. Keep a prediction and a falsifier with the experiment.

## Integrate raw-accepted source

Review source and original context, then use the existing journaled promotion
path for a function that meets the raw gate:

    python scripts/promote.py <id> --source candidates/<candidate>.c --verify-only
    python scripts/promote.py <id> --source candidates/<candidate>.c

Promotion protects established matches and checks the selected source unit.
Inspect the source before integration: exact bytes are not permission to insert
copied code, original-binary dispatch, unsupported assembly, or guessed behavior.
Recover any interrupted promotion before starting another one.

For raw-equal functions already in a fully inventoried canonical unit, use
`python scripts/promote.py <ids...> --existing-unit --verify-only`, then omit
`--verify-only` to publish. This status-only path keeps the source in place and
rechecks RAW, RELOCATED and legacy linked peers. Mixed or incompletely inventoried
units can remain blocked even when one contribution is raw-equal; retain that
diagnostic result until the unit ownership has been reviewed.

## Revalidate and inspect source closure

Run the RAW and RELOCATED regression gate after canonical source changes:

    python scripts/match.py verify --report work/strict_verification.json

Use the module's focused source-graph and differential entry points to expose
missing providers and test semantic claims. For example, TITLE and ENG1 have
their own documentation and source-graph commands. A successful link establishes
closure only for the inputs that were selected. Diagnostic placeholders do not
count as reconstructed gameplay and must stay out of production builds.

For existing source with exact code and reviewed reference identities:

    python scripts/match.py references <id> --bindings evidence/relocated_reference_bindings.json

    python scripts/promote.py <ids...> --relocated --bindings evidence/relocated_reference_bindings.json --verify-only
    python scripts/promote.py <ids...> --relocated --bindings evidence/relocated_reference_bindings.json

Use the same command without `--verify-only` only after reviewing those independent
bindings. It retains source in its real translation unit and protects all accepted
peers. Masked equality and implied addresses alone cannot supply bindings.

The ordinary module/build and diagnostic commands are:

    python scripts/linked.py build TITLE.DLL
    python scripts/eng1_behavioral.py --output work/eng1_behavioral/build
    python scripts/linked.py verify
    python scripts/match.py verify --raw-only
    python scripts/similar.py verify
    python scripts/census.py --recorded --output work/census/recorded.json
    python scripts/convergence.py --output work/coverage.json
    python -m unittest discover -s tests -q

TITLE has a complete reviewed source graph. ENG1 deliberately fails its genuine
link while dependencies remain missing. ENG3 and the host need reviewed complete
link plans before claiming closure. ENG1 `--include-incomplete` is an explicit
diagnostic option: generated fail-on-entry bodies live only in ignored work/ and
never establish source completeness. Use bounded differential fixtures for actual
semantic changes; do not launch autonomous gameplay.

The recorded census freshly compiles each source/ordered-flags profile and
separates raw equality, reference-only differences, real code differences, and
different function extents. It is diagnostic: reference-only results do not prove
symbol identities and cannot promote matching state.

## Durable results

Keep compiler/toolchain provenance, commands, source identity, prediction,
falsifier, and concise results with the relevant evidence. Keep full reports,
objects, binaries, and scratch sources ignored. Update recovery.json only through
the accepted workflow. Preserve every earlier raw match in the relevant
regression gate.

See [matching proof rules](matching.md) and the repository [contract](../AGENTS.md).
