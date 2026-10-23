# Current handoff

The direction is one faithful native source game, eventually built on SDL3.
Original PC binaries remain the evidence oracle; original first-party EXE/DLL
modules will not be runtime dependencies of the finished game.

Start with the [project direction](project_direction.md), [working contract](../AGENTS.md),
[matching rules](matching.md), [workflow](workflow.md), and recovery.json. The
ledger decides accepted function status. Dated evidence reports preserve
measurements for their specific source and toolchain inputs; they are not a
current task queue or current coverage census.

Existing FUNCTION_MATCH claims remain under the raw-byte gate. The new
FUNCTION_MATCH_RELOCATED definition requires exact non-reference code and
independent reference identity checks. Both are checked by `scripts/match.py verify`.
The `scripts/promote.py --relocated --bindings ...` command provides journaled
adoption of reviewed existing source.
Masked or constrained-link results alone are not acceptance.

Continue source, semantic, and dependency reconstruction using those gates.
Prefer real missing implementations and shared-state or ABI corrections in coherent dependency families. Keep source completeness,
matching, semantic evidence, source-link closure, and runtime behavior distinct.
A module that links is not necessarily behavior-complete, and source that is
valuable for integration must not be labeled as matching without proof.

Historical dated documents and their evidence files remain provenance. Review
their hypotheses against current source and tools before reusing a command.
Current realignment: branch `codex/function-matching-realignment` contains main
`c12c7fc` and the earlier `6640420` branch work. There are 335 accepted function
rows / 26,512 bytes: 135 RAW rows, 43 retained linked rows and 157 new
RELOCATED rows / 12,800 bytes. Two of the linked rows retain earlier raw proofs,
so `match.py verify --raw-only` checks 137 proofs without double-counting rows.
All 134 old RAW rows remain accepted; `exe.host_0a190` adds one raw match / 16 bytes.

The fresh 2,211-row / 502,791-byte source census leaves 1,667 unaccepted rows with
reference-only differences, 177 with code differences, 13 with extent differences,
and 19 raw-equal diagnostics awaiting unit ownership/inventory review. Those counts
are diagnostic categories, not additional matching acceptance or whole-game coverage.

TITLE's 362 mapped source functions link. ENG1 has 933 compiled mapped providers
out of 1,231 diagnostic spans and 763 genuine unresolved symbols (459 function,
304 data/untyped); no generated placeholder is in that build. Nineteen measured
setup-dispatch bindings closed real dependencies. Ninety-nine other implemented
functions were removed from stale provisional classifications. ENG3 and the host
still need complete reviewed link plans; current source rows are 555 and 403.

Next: recover the remaining neighboring ENG1 player mode handlers at `0x18a10`,
`0x18ba0`, `0x18c30`, `0x18ed0`, `0x19140`, `0x191e0`, `0x19260`, `0x19290`,
`0x193f0`, `0x19480`, and `0x19c20`, then their data dependencies. For matching,
verify callback-table/BSS ownership before expanding symbolic-reference support;
do not retune reference-only functions. See `evidence/realignment.json` for the
frozen commands, counts and limitations of this consolidation.
