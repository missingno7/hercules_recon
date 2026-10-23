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

## Revalidate and inspect source closure

Run the raw regression gate after canonical source changes:

    python scripts/match.py verify --report work/strict_verification.json

Use the module's focused source-graph and differential entry points to expose
missing providers and test semantic claims. For example, TITLE and ENG1 have
their own documentation and source-graph commands. A successful link establishes
closure only for the inputs that were selected. Diagnostic placeholders do not
count as reconstructed gameplay and must stay out of production builds.

The relocated-match verifier and promotion command are still being integrated.
Until their interface is finalized, masked or linked diagnostics remain leads;
do not record them as accepted relocated matches.

## Durable results

Keep compiler/toolchain provenance, commands, source identity, prediction,
falsifier, and concise results with the relevant evidence. Keep full reports,
objects, binaries, and scratch sources ignored. Update recovery.json only through
the accepted workflow. Preserve every earlier raw match in the relevant
regression gate.

See [matching proof rules](matching.md) and the repository [contract](../AGENTS.md).