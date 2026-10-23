# One Hercules reconstruction

The goal is a complete, faithful native source port of Disney's 1997 Windows
Hercules game, eventually using SDL3 and modern compilers. Original PC binaries
are the evidence oracle; PSX builds are secondary evidence. The finished runtime
must not depend on the original first-party executable modules.

Historical matching is a foundation, not a requirement to reproduce the
original linked image. The linker may choose different addresses. Renderer,
host, emulation, and behavioral work are discovery methods that feed the same
implementation and evidence base.

## One project, separate proof dimensions

Keep these questions separate:

| Dimension | Question |
| --- | --- |
| Source coverage | Is the actual first-party implementation present, or is work still missing/provisional? |
| Matching | Does fresh historical compilation produce exact function code, with references proved where needed? |
| Semantics | Does original-backed evidence support the behavior and state transitions? |
| Dependency closure | Do the selected source units link without unresolved providers? |
| Runtime completeness | Does the complete game start and play through the required paths? |

A positive answer in one row does not imply the others. In particular, a normal
link does not prove matching, whole-image equality, or semantic completeness.
Finite comparisons and GPU screenshots establish only the paths they measure.

recovery.json remains the authoritative function-matching state. Existing
FUNCTION_MATCH entries retain their raw-byte proof. FUNCTION_MATCH_RELOCATED
is the accepted category for exact compiler-generated instructions with every
linker-resolved reference independently verified by match.py and the journaled
promote.py acceptance path. CODEGEN_SIMILAR remains diagnostic. Semantic
evidence and source coverage do not silently promote a matching row.

## Direction of work

Prioritize real source recovery, shared state, ABI and data ownership, and
coherent dependency families. Use exact matching where the evidence supports
it, while keeping nonmatching but useful source visibly classified. Do not spend
time reproducing original RVAs, object order, PE sections, padding, import order,
or whole-image bytes as independent milestones.

Keep original gameplay logic separate from platform implementations. Add small
platform boundaries when recovered call paths require them. Preserve the
original update order, integer behavior, RNG, actor and collision state, resource
semantics, and renderer command ordering. Modern rendering may use a different
backend while respecting the recovered interface.

renderer_contract preserves evidence-backed original interfaces and reference
utilities. dropin_renderer remains a diagnostic/integration experiment rather
than a permanent architecture commitment. Retain useful host and renderer
findings; judge experimental code by whether it still reduces reconstruction
risk.

## Integrity

Never modify original assets or binaries. Do not use copied machine code,
instruction patching, original-binary dispatch, fake gameplay, or arbitrary
success stubs to claim source coverage or matching. Keep diagnostic placeholders
out of production builds. Do not start autonomous gameplay-playing agents.
Prefer static reconstruction, bounded original/source differentials, and
human-recorded end-to-end regressions.

See [the reconstruction goal](full_matching_goal.md), [matching proof rules](matching.md),
[workflow](workflow.md), and the [working contract](../AGENTS.md).