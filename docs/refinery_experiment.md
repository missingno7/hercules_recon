# Bounded near-match refinery, 2026-10-07

The result is **mixed support, not a demonstrated broad speedup**. A first-pass
twelve-function region produced three raw equals and five relocation-only
diagnostic equals. Its single localized near candidate became raw exact with
the first generated variant. None of the four calibration targets converged.
One difficult calibration target improved substantially. This justifies a small,
diagnosis-driven compiler search, not an unrestricted search or a percentage rule.

The previous [macro experiment](macro_experiment.md), its source, and every
recorded wave remain unchanged. The compact [measurement ledger](../evidence/refinery.json)
pins source/tooling identities, all development candidates, replay results, and
the blind baseline. `recovery.json` alone records canonical acceptance.

## Architecture and safeguards

`scripts/refinery_forms.py` describes bounded families, not individual compiler
guesses. `scripts/refinery.py` materializes one changed function in the existing
translation unit under ignored `work/`, deduplicates identical source strings
within a family, freshly compiles with pinned VC5 RTM `/O2 /Gy`, and reuses the
unchanged strict oracle plus macro diagnostic comparison. No compiler flags vary.

Every attempted variant separately compiles and runs a historical C semantic
harness. All other defined functions must preserve their complete COFF data,
relocations, and section properties. That freezes raw equals, relocation-only
equals, and even unrelated unresolved candidates. Relocations are resolved only
in a diagnostic memory buffer, using the existing independently checked symbol
map; no executable or object is patched.

Retention requires stable relocation symbol/type multiplicities and no regression
in normalized differing bytes, literal instruction agreement, instruction-count
distance, ordered CFG equality, or direct-call equality, with at least one gain.
Raw equality has priority. Relocation offsets and mismatch spans are recorded for
local diagnosis, not treated as acceptance proof. This is deliberately conservative:
a potentially useful tradeoff may be rejected. It is not a global optimization
algorithm or a proof of semantic equivalence. The existing macro comparator's
90% instruction label is retained for reproducibility; it is **not a learned
success threshold**. Human classification also recognizes the localized SIB case
despite its lower literal instruction percentage.

The search stops on raw equality or diagnostic equality. The latter enters the
linkage queue. All attempted artifacts remain ignored for audit; only improvements
are designated as retained results. Existing canonical source is never a search
output. Function replacement supports these reviewed C forms, not arbitrary C
syntax or automatic discovery of legal transformations.

## Calibration

| Target | Development variants | Baseline builds | Differing normalized bytes, start → best | Result |
|---|---:|---:|---:|---|
| `change_kind` | 30 | 3 | 8 → 8 / 208 | No retained gain |
| ENG3 `measure_relative_vector` | 12 | 1 | 2 → 2 / 224 | No retained gain |
| `create_command` | 12 | 1 | 55 → 55 / 96 | CFG-regressing gains rejected |
| `find_tag` | 16 | 2 | 42 → 10 / 64 | Retained, still unequal |

There were 70 development variants plus seven baselines, each with a separate
semantic build. Eight early `change_kind` forms were repeated in an expanded
family and are explicitly counted as spent work, not new coverage. The final
calibration replay uses 62 variants plus six baselines: 68 target compilations
and 68 semantic compilations. All source hashes and retained metrics reproduced.

The model proposed families and inspected results at family boundaries. It did
not hand-author each of the 70 candidates:

* `change_kind`: inline versus 16/32-bit masked index; index assignment before or
  after the kind store; indexing, pointer addition, byte-offset addressing, and
  commuted base/offset spelling. A follow-up varied meaningful table-pointer
  materialization before/after index computation. Table globals are reloaded at
  every original read so storing category/image cannot silently change alias
  behavior. All natural forms tested either emitted the same eight SIB differences
  or worsened code generation. Baseline instruction agreement is 85.965%, although
  byte agreement is 96.154% and both sides have 57 instructions. Eight one-byte
  address-encoding differences therefore affect eight literal instruction tokens.
  Eleven relocations independently prevent object-only raw acceptance.
* ENG3 relative vector: both shift-term orders, both addition groupings, meaningful
  intermediate terms, and explicit `int`/`short` narrowing. Ten forms keep the same
  two immediate differences; two wider temporary forms worsen allocation. ENG1
  canonical source remains exact and unchanged; the other two functions in the
  experimental translation unit remain frozen. A single byte contribution cannot
  equal the two different oracles simultaneously. No successful module-specific
  source or translation-unit explanation was found; further context work belongs
  in reasoning, not repeated algebraic permutations.
* `create_command`: cached flags, direct return versus early/late meaningful result
  local, and chained versus individual word clearing. Some forms reduce differences
  from 55 to 52 but change the CFG, so they are rejected. This is not convergence.
* `find_tag`: index versus pre/post cursor, `for`/`while`, and increment before/after
  testing. The first pre-cursor/before-test variant improves instruction agreement
  from 60.870% to 95.652% while preserving 23 instructions and CFG/calls. The remaining
  ten bytes reflect moving cursor initialization across the count branch. A second
  four-form family scopes initialization inside a positive-count guard or after
  an early return, with `for`/`do` traversal; all regress structure and are rejected.
  The hard control was mechanically improvable, but not mechanically finished.

Macro variants run the existing 40-check synthetic harness. Vector variants run
67,719 shared semantic checks, including historical short arithmetic behavior.
These tests are finite evidence, not proof for every alias or input. No original
game module is loaded or executed. No assembly, dummy locals, volatile, artificial
padding, machine-code arrays, forced placement, or compiler/tool patches were used.

## Blind region

The region is ENG1 `0x216b0..0x219df`, twelve contiguous actor/effect callbacks,
816 bytes. Shared evidence includes actor offsets, fixed-point coordinates,
frame/tick fields, effect flags, script globals, and external random/frame/sound
calls. It was selected from disassembly before any compilation of its source.
Neighboring entries and terminal returns/NOP alignment bound the contributions.
Addresses and hashes are in [the blind manifest](../evidence/refinery_blind.json).

This is blind to prior compile results, not to the target disassembly. Selection
favored a tractable contiguous callback region rather than switch-table-heavy
regions encountered in the initial survey. Thus it is not a random sample of the
game. `calibration/refinery_blind.c` preserves the entire first pass without tuning.

| Classification | First pass | After mechanical refinement |
|---|---:|---:|
| Raw exact | 3 / 144 bytes | 4 / 192 bytes |
| Relocation-only diagnostic equality | 5 / 368 bytes | 5 / 368 bytes |
| Localized near codegen | 1 / 48 bytes | 0 |
| Same-CFG hard allocation/source form | 2 / 144 bytes | 2 / 144 bytes |
| Structural mismatch | 1 / 112 bytes | 1 / 112 bytes |

Immediate raw equals: `initialize_backdrop` (64), `initialize_particle` (32), and
`wrapped_delta` (48). Relocation-only: `initialize_flicker`, `tick_flicker`,
`next_script_frame`, `reset_effect`, and `tick_burst`.

`initialize_burst` starts at 95.833% bytes and 90.909% instructions with identical
11-instruction CFG/call structure. Its only differences are two store offsets:
the chained zero assignment stores the independent fields in reverse order.
The model selected a three-form family (separate stores, reversed chain, reversed
separate stores). The generator's first form, separate stores, is **fresh raw exact
for all 48 bytes**, with no relocations. Search stopped before compiling the other
two. Eleven surrounding contributions stayed byte/relocation identical.

This is one family intervention after the first-pass classification, one compiled
variant per conversion, and two target builds including its search baseline.
The whole-region first-pass build is additional. Every semantic build passes
3,392 checks covering wrapped coordinates, visibility thresholds, random branches,
script wraparound, animation state, and callbacks. The immutable baseline and
conversion were freshly replayed with the same result. No manual per-function
tuning preceded classification.

The reasoning queue contains `tick_particle` (scheduling/register differences),
`initialize_sound_effect` (several field-store/register lifetimes), and
`maybe_start_effect` (different branch/tail structure). They receive no blind
search budget simply to inflate the conversion count. The four raw-equal blind
functions remain experimental, with reproducible evidence for later promotion.

## Linkage closure is a different problem

The original macro's five relocation-only functions are unchanged:
`detach_attachment`, `detach_child`, `detach_buffer`, `insert_scene`, and
`hide_and_unlink` (352 bytes). Neither this group nor the five blind relocation
equals received source-search variants. Diagnostic relocation equality remains
strictly separate from `FUNCTION_MATCH`.

The smallest useful next link experiment is the REL32-only `hide_and_unlink` plus
its already exact `unlink_12c` callee. Compile their ordinary C and link with the
pinned historical linker, inspecting the actual emitted call displacement,
contribution order, and full spans. Identify the real intervening contributions,
object grouping, and linker behavior if the displacement differs; do not add
padding or force addresses. A freely linked two-function image alone may not
recover the original separation and therefore is not guaranteed to prove equality.

For detach helpers, the reconstructed release routines and their dependencies
must also close naturally. `insert_scene` additionally needs the original data
contribution layout for the global anchor. Linker maps/relocations and fresh image
hashes must substantiate any later claim. The current gate accepts relocation-free
COFF contributions only, so even a successful exploratory linked-image comparison
would remain separately documented until a reviewed linked-image gate exists.
No acceptance-gate redesign is included here.

## Project gains and verification

Six previously measured macro functions were extracted without changing their
contributions: `reset_frame_state`, `advance_frame`, `restart_steps`, `select_frame`,
`count_chain`, and `append_chain`. A separate 595-check frame/list harness passes.
The unchanged normal promotion gate freshly verified every accepted function and
all six candidates, checked complete object coverage, and promoted them together
to `src/shared/frame_state.c`. It now records **32 accepted instances, 23 source
functions, and 3,984 mapped bytes**. This adds six accepted instances / 288 bytes;
those bytes were already experimental equals, so they are not new search discoveries.

The blind pass discovers four experimental raw equals / 192 bytes, of which
one / 48 bytes is attributable to mechanical refinement. No calibration function
converts to raw or relocation-only equality. Previously accepted ENG1 vector and
all other canonical matches remain exact. All twelve immutable original-file
hashes verify. Original macro source/evidence hashes remain unchanged.

Regression checks passed: 21 Python gate/metric/ranking tests, all 99,333 existing
semantic checks, the 595-check canonical extraction harness, and the blind suite.
Ranking tests explicitly reject byte improvements that regress CFG, calls,
instruction count/agreement, or relocation identity. The historical five-wave
macro replay remains a separate reproducibility check.

## Effort, routing, and interpretation

Development calibration loops took about 107 seconds inside the measurement
runner, including semantic builds. The blind refinement loop took about two
seconds, including its baseline and both semantic builds. These are tool-loop
times, not total agent time. Source reconstruction, tooling, test authoring,
review, replay, promotion, and reporting dominate the broader task. Token usage
is unavailable; there is no trustworthy comparison to a controlled alternative.
The measured task interval through validation/report preparation was approximately
26 minutes (13:54–14:20 UTC), excluding the final Git publication step.

Eight family-level interventions are counted: four initial calibration families,
one index-order expansion, one table-pointer follow-up, one cursor-guard follow-up,
and the blind zero-store family. That excludes infrastructure and semantic reasoning.
Across development, 71 source variants (70 calibration plus one blind) buy one new
raw-exact tail. The earlier macro experiment used five development compilations
and eight local interventions, but addressed different functions and had no
automated semantic build per variant. These are not comparable speed measurements.
The current workflow **does not demonstrate materially lower end-to-end cost**.

A numerical threshold is unsupported: 96.154% and 99.107% byte cases stalled,
95.833% converged, and 34.375% improved without convergence. A practical rule is:

1. Freeze raw equality. Queue normalized-only equality for linkage.
2. For a structurally corresponding case, inspect mismatch localization and name
   a credible source family. Independent store ordering is a demonstrated cheap
   case. Cursor induction merits a small probe even with low literal similarity.
3. Give one small predeclared family a budget, typically the observed 3–16 forms.
   This range is an operational cap from this experiment, not a measured optimum.
4. Keep only multidimensional gains. Stop at equality. If a family is exhausted,
   return to reasoning; require a new evidence-based diagnosis before another
   family. High similarity alone does not justify more variants.
5. Route branch/semantic disagreement and unexplained allocation to reasoning.
   Use multi-module evidence to constrain hypotheses, not to merge unequal targets.

The generalizable parts are the classification, frozen surroundings, explicit
semantic harnesses, bounded family generators, fresh compiler oracle, conservative
retention, stop rules, and separation of relocation closure from source codegen.
The successful store-order pattern is plausibly reusable; its hit rate, these
budgets, and VC5's SIB/shift scheduling behavior cannot be generalized from one
small validation region. The strongest confirmed leverage here is eliminating
source search for ten relocation-only functions across the two regions.

## Reproduction

Run from the repository root with the pinned tools installed; build outputs,
candidate sources, semantic executables, and full attempt logs stay ignored.

```powershell
python scripts/refinery_calibrate.py
python scripts/refinery_blind.py
python scripts/macro_compare.py --replay evidence/macro_actor.json
python scripts/match.py verify --report build/verification.json
python scripts/test_pilot.py --suite all
python -m unittest discover -s tests -p 'test_*.py'
python scripts/inventory.py --verify
```

Calibration replay writes `work/refinery_replay/`; blind replay writes
`work/refinery/blind/`. The retained tag source is produced by `pre-for-before`;
the retained blind source by `separate`. The ledger records their source hashes.
Original macro waves and canonical recovery are never edited by these replay tools.
