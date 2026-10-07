# Bounded macro reconstruction experiment

The experiment supports a coherent first reconstruction pass, but does **not**
demonstrate materially faster final matching. After the initial pass, the useful
changes were local source-form corrections rather than a shared type correction
that solved many functions at once. Recommendation for this region:
**USEFUL ONLY FOR EARLY RECONSTRUCTION**.

## Region and starting state

ENG1 DLL RVAs **0x29160–0x296df** contain 18 contiguous animation-state, object-list
and attachment-cleanup functions: 1,408 bytes including natural padding. The
functions share one object layout and relationships between animation tables,
command handles, two linked lists, callbacks and ownership flags. They include
loops, allocation, cleanup, global tables and direct/indirect calls; they are
game code rather than startup or CRT internals. This is a coherent subsystem
slice, **not a proven original translation unit**. No historical filename, PCH,
debug symbols or original linker map identifies such a unit.

At baseline commit `dd71c0e`, four region functions were already accepted:
two frame-motion functions and two unlink functions, totaling 400 bytes. Those
four definitions occupied 42 lines across two partial object views. They were
reused as controls and are excluded from new-recovery counts. Fourteen functions
had no accepted reconstruction. Some disassembly had already been surveyed;
this was not a blind trial. Previous agent time and token usage are unavailable.
Project-wide canonical acceptance was 26 instances / 17 distinct functions.

The initial experiment was one 265-line, 7,170-byte C file, compiled before
individual tuning. The final file is 271 lines / 7,275 bytes. One `Actor` view
unifies the measured offsets; table record/callback strides are declared once.
External globals and procedures remain symbolic declarations, with addresses
only in the comparison manifest. No original binary can be called through this
source, and no source address bindings or executable patches were introduced.

## Oracle and measurement

The primary oracle is immutable `assets/pc/HERCULES/ENG1.DLL`, SHA-256
`01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757`.
The installed DLL equals the installer copy; the separate no-CD EXE is not used.
Compilation reuses the pinned VC5 RTM CL 11.00.7022 toolchain and existing
`compile_source`, `COFF`, `PE` and strict `compare` implementations: `/c /Gy /O2`,
32-bit x86, ordinary C calling conventions. Caller stack cleanup supports cdecl;
the full historical declarations and memory/packing options are not uniquely
identified. PE linker 5.0 and static Microsoft runtime evidence remain as recorded
in [toolchain notes](toolchain.md). This experiment does not link a game image.

`macro_compare.py` adds diagnostics without changing the acceptance gate:

- Raw equality compares complete function contributions, including padding,
  and rejects unresolved COFF or PE relocations.
- A disposable comparison buffer resolves mapped REL32/DIR32 operands. Mapping
  destinations are checked against original calls/base-relocation operands.
  This does not produce executable code or confer `FUNCTION_MATCH`.
- Instruction similarity uses matched SequenceMatcher tokens divided by the
  larger instruction count. Trailing NOPs are excluded; registers and operands
  stay literal except local jump direction. It is not semantic equivalence.
- Ordered CFG equality compares block edges and conditional-branch predicates.
  Direct call destinations and indirect operands/counts are reported separately;
  callback destinations at runtime remain unknown. CFG equality is not proof of
  the entire function's behavior.

## Measured waves

All 18 functions compiled in every wave. Counts in the exact column include the
four controls; normalized equality also includes raw-exact contributions.

| Wave | Raw exact | New exact | Exact bytes | Normalized equal | Ordered CFG equal | >90% instruction similarity |
|---|---:|---:|---:|---:|---:|---:|
| Before candidate | 4 known | 0 | 400 known | not measured | not measured | not measured |
| 0: coherent first pass | 9 | 5 | 608 | 11 | 15 | 13 |
| 1: ownership/alias forms | 9 | 5 | 608 | 13 | 16 | 14 |
| 2: loop-state forms | 10 | 6 | 688 | 14 | 18 | 15 |
| 3: bounded local tail | 10 | 6 | 688 | 15 | 16 | 15 |
| 4: restore regressed forms | 10 | 6 | 688 | 15 | 18 | 15 |

Initially, four non-exact functions exceeded 90% instruction similarity, three
had different CFG shapes, and two more had lower similarity despite equal CFG
shape. Those categories do not establish semantic correctness or incorrectness.
The final raw-exact contributions cover 688/1,408 bytes, of which **288 bytes in
six functions are new**. Most of that gain arrived immediately: five new exact
functions / 208 bytes in wave 0; convergence added one / 80 bytes.

Final normalized equality covers 1,040/1,408 bytes. Sixteen functions exceed 90%
normalized byte similarity, including ten raw-exact functions. Aggregate
normalized byte similarity rose from 72.569% to 92.543%; raw positional byte
similarity rose from 69.444% to 87.429%. These aggregates use the larger original
or candidate size per function, so shifted instructions are penalized.

Wave 1 changed two ownership expressions: select the attachment owner before
clearing it, and read the scene successor from the just-written object field.
Both became relocation-only diagnostic equals; neither created a raw match.
Wave 2 retained the frame-loop sentinel state and snapshotted the registry count.
Both improved; the frame function became raw-exact and registry CFG shape agreed.
These are related **local edits**, not one systemic fix applied across the batch.
The shared layout needed no correction after the baseline, so this trial contains
no evidence of a shared type fix making many functions exact simultaneously.

The tail tested four independent hypotheses. Branch-local kind stores and
separate link clears helped. A command-result local and an indexed registry loop
regressed CFG shape and were restored. By normalized byte difference alone,
wave 3 improved three functions and worsened one; by CFG shape it worsened two.
Restoration improved one byte score and worsened another slightly while restoring
both CFGs. The ledger keeps those dimensions separate. No raw-exact contribution
ever regressed.

## Remaining problems and limits

Kind dispatch has eight differing SIB bytes: the same 57 instructions use
commuted base/index register sums. It is 96.154% equal after relocation resolution,
but remains non-exact. Command creation and registry traversal retain allocation
or induction-variable differences (73.333% and 60.870% instruction similarity).

Eight functions contain relocations. Five of those otherwise match completely,
but the current object-level strict gate cannot accept them without a naturally
reconstructed link/data layout. That limits conclusions about final convergence:
the experiment reached raw equality for all ten relocation-free functions,
while link closure was outside this bounded slice. Similarity is not substituted
for missing linkage evidence.

There were five development compile/compare rounds, four substantive reasoning
passes plus rollback, eight function-local interventions across seven functions
(four interventions in the isolated tail), one agent, and no workers. Five additional fresh builds reproduced the
saved waves exactly. The instrumented region-construction-to-replay window was
about 17 minutes; it excludes initial survey and subsequent reporting/final
checks. Token usage and a controlled function-by-function timing baseline were
not recorded. A speedup, token saving or agent-overhead reduction is therefore
**not measured**.

## Assessment and possible workflow changes

A shared-file pass avoided rediscovering the object layout and made representation
consistency easy to check across 18 functions. It produced five new raw matches
and two relocation-only equals from one compiler invocation. It also exposed
which failures involved loops, ownership, address encoding or unresolved linkage.

It did not remove the final independent problems. Traditional per-function work
would still need substantially the same alias, loop and allocation reasoning;
the trial provides no defensible estimate of how many iterations it would save.
Measuring/debugging the batch also required additional diagnostic tooling. The
four known controls and existing object-layout knowledge favor the macro result.
No claim about 30- or 50-function batches follows from this 18-function slice.

For a follow-up trial, use **10–20 genuinely related functions** with a shared
representation and a stated link-closure boundary. An inferred original TU is a
useful hypothesis only when independently supported, not a required unit of work.
Switch to smaller groups or individual cases once common layouts are stable and
mismatch classes become local—here, after wave 2, when all ordered CFGs agreed.

If adopted later: retain one shared candidate, compile once per macro wave, keep
per-function raw and diagnostic metrics separate, freeze exact contributions,
and switch granularity after one or two waves without a common root cause.
Do not raise the acceptance threshold or restructure canonical code. A second
region and a timed comparison would be needed before claiming faster convergence.

## Reproduce and current project state

The final candidate is [macro_actor.c](../calibration/macro_actor.c). The
[ledger](../evidence/macro_actor.json) contains reviewed spans, symbolic reference
maps, hashes, current results and reversible source edits. Older source variants
are reconstructed from that ledger; full disassemblies and objects stay ignored.

```powershell
python scripts/macro_compare.py --replay evidence/macro_actor.json
python scripts/test_pilot.py --suite macro
python scripts/match.py verify --report build/verification.json
python -m unittest discover -s tests -p 'test_*.py'
```

All five waves reproduced under fresh historical compilation. Forty synthetic
cross-function checks passed without running any original executable or DLL.
All 26 canonical matches remained exact; all 99,333 registered semantic checks,
17 Python tests and 12 immutable-original hash checks passed. The diagnostic
tests verify CFG predicate sensitivity, call-target identity, wave reconstruction
and rejection of modified source. Existing canonical source
and `recovery.json` remain unchanged: **no experimental result was promoted**.

**USEFUL ONLY FOR EARLY RECONSTRUCTION**
