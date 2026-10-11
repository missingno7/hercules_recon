# Blood II methodology transfer, 2026-10-10

The useful next step is to complete real contributions and localize shared compiler
questions. Hercules already implements most of Blood II's experimental workflow.
No additional original binary, new dependency, compiler change, placement control
or relaxed acceptance rule is needed for this pilot.

This audit inspected Hercules at `dce228831d759985ab2c4f9df054b0762a7411cd` and
the local Blood II checkout reporting HEAD
`d71e4788b9befec6d8a4666c11812ef18264ab17`. Seventeen inspected Blood II files
match HEAD after newline normalization. Its worktree status command fails; this
is not a claim that its entire checkout is clean. Blood II results are reported
repository evidence, not independently reproduced builds. Private citations and
file hashes: `work/transfer_audit/blood2/audit.md` and `provenance.json`.

## Audit: reuse before adding

| Technique | Existing Hercules implementation | Remaining useful increment |
|---|---|---|
| Compiler fingerprinting | `toolchains/manifest.json`, `match.verify_toolchain`, `calibrate_control.py`, exact anchors; RTM `/O2` and SP2 controls | Reproduce specific front-end/back-end questions; no blind revision/flag sweep |
| Minimal experiments and allocation | TITLE C/C++ probes, `count_scan.py`, W1/A1/K1/K2 model in `title_stage1.json`, live-read studies | Small paired experiments for constant pieces, register interference and join reloads |
| TU recovery | `object_map.py`, `unit_bounds.py`, literal/data order, C versus C++, shared type headers | Finish real missing definitions before revisiting declaration-count ties |
| Relocation diagnostics | `masked.py`, `symbol_table.py`, `title_link.py` data-unit comparisons, `dependency_link.py` | Aggregate literal/import/static-owner checks; actual linked raw comparison later |
| Strict promotion | Fresh compilation, full raw contribution including padding, target hashes, all-peer regression, journal and lock | Retain the gate; linked acceptance is a separate future implementation |
| Evidence and negative families | `region.py`, explicit region index, immutable recipes, context hashes, `region_knowledge.json` | A small shared-family reading list with current recovery overlay |
| Byte prioritization | Region Pareto display, leaf survey, module summary | Derived cross-module byte ceilings, without a scheduler, difficulty score or routing ledger |

Blood II's normalized strict classifications are stronger than simple masking
because they check bindings, but they still differ from Hercules's actual raw
byte equality. They must not become `FUNCTION_MATCH` here. Likewise its VC5 SP3
and Intel findings do not identify Hercules's RTM rules. Several old TITLE B1/B2/B6
summaries are superseded by later observations and recovery rows; the screen-handler
B6 family is resolved. Read current context before replaying its old attempts.

## Prioritized implementation plan

| Priority | Work | Expected effect and acceptance boundary |
|---|---|---|
| 1 | Complete TITLE's real data owners and source contributions, using the existing natural-link plan and full-unit diagnostic | Highest strict-byte leverage: 80,928 currently masked bytes await correct context and real addresses. This is a ceiling, not a promised gain or an accepted link. |
| 2 | Keep an independent ENG3/ENG1 missing-source lane active, with PC actor/resource/interface families | Expands the much thinner engine source coverage while a compiler question is blocked. Reuse shared layouts only where target reads/writers support them. |
| 3 | Investigate shared scratch phase and join reloads in minimal controls | Existing TITLE span ceilings are 10,096 and 8,320 bytes respectively. A discovered mechanism may help several functions, but similar residues alone do not prove one cause. |
| 4 | Resolve constant webs under real interference, then apply to tally/configuration and shared reset scheduling | The two call-bearing TITLE leads cover 4,128 bytes; the separate call-free reset family spans 816 bytes across three DLLs. Do not generalize call-lifetime results to reset scheduling. |
| 5 | Finish genuine TU/header/type context and jointly measure every owned function | Avoids independent count hits that cannot coexist. Reuse each compile when adding a joint count diagnostic; padded declarations never become production source. |
| 6 | Add aggregate literal/IAT/static binding diagnostics when another concrete mismatch justifies them | Finds wrong data content and ownership before natural placement. Extend `symbol_table.py`/`title_link.py`; do not build a second verifier or recovery ledger. |

The latest full-unit evidence in `title_closure.json#full_link_2026_10_10` reports
all function bodies present as canonical rows plus private drafts, with remaining
unresolved data. Draft size differences at `0x54f0`, `0x9c90`, `0x186e0` and
`0x1a2c0` shift subsequent code. Finishing a unit also changes count-sensitive
drafts and exposes type conflicts. The newly recovered object-configuration
data unit is already present; do not recreate it. Some initialized ranges remain
unowned, including data with no known user, so placeholder globals are not a fix.

That evidence also supports testing `/OPT:REF /OPT:NOICF` and OLDNAMES ordering.
The reusable canonical `title_link.py` still uses its older `/OPT:NOREF` diagnostic
configuration. Reconcile those hypotheses against a fresh complete contribution
inventory before changing defaults. A `/FORCE:UNRESOLVED` layout image supplies
no raw acceptance, even where an individual contribution happens to be equal.

## Cross-module evidence and limits

`shared_functions.json` already proves complete first-party contributions shared
by TITLE/ENG1/ENG3: command-bit clearing and both unlink routines. Motion and
count-removal families are shared by ENG1/ENG3. The relative-vector variant swaps
two shift immediates and must remain separate. The 272-byte reset family occurs
at TITLE `0x94c0`, ENG1 `0x27fe0`, ENG3 `0x1d8a0`; use it as a common scheduling
control. Library maps separately identify CRT/MFC contributions; they are not
recovered gameplay. The EXE's MFC-heavy interleaved code makes a simple prefix map
inappropriate for whole-image missing-code totals.

Archive/resource descriptor readers and the RLE/frame emitters already expose
data layouts (`archive_name_contract.json`, `resource_metadata.json`,
`resource_record.h`, TITLE emitter evidence). They support bounded offsets,
widths and lifetime hypotheses, not complete historical types or asset formats.
The new ENG3 pilot independently observes a count-prefixed actor-pointer vector:
writer `0x1bd11` increments its count and stores a pointer at count*4; dispatch
`0x13a0` reads actor byte `+0x97` and selects a callback. This evidence requires
neither a new binary nor PSX source ancestry.

## Implemented pilot

1. `similar.check_rows` now checks recorded whole-oracle hashes before and after
   masked comparison and rejects a source changed during compilation/verification.
   Unit tests exercise changed-oracle and changed-source refusals. The module
   summary also refuses a map from another module; ENG3's default summary no longer
   accidentally attaches TITLE totals. Raw promotion code is unchanged.
2. `convergence.py` derives strict, masked, legacy similar, reconstructed and
   untracked-map counts. `convergence_families.json` is a research reading list,
   not recovery state. Closed codegen peers are removed using current rows;
   stale/overlapping extents are refused. Family rankings use unresolved span
   bytes only; overlapping families are not additive and unknown extents stay
   explicit. An untracked span can already have a draft and is not necessarily
   missing source. Source absence and proven behavior need separate review.
3. Eight small fresh RTM probes isolate constant pieces around calls. The reusable
   `probe_constant_lifetime.py` then rebuilds a two-condition control pair twice
   with identical declarations. With calls, one `mov esi,6` feeds eight stores
   (96-byte contribution); without calls it uses `mov eax,6` (80 bytes). Both
   repeated complete contribution/noncode identities agree. This narrows the
   open tally question to web interference/splitting in its real context; it does
   not explain the original allocation or establish a target match.
4. A separate bounded ENG3 dispatch-source family is retained in private scratch
   with its failed predictions and measured residuals. Its final 64-byte output
   has 30 non-relocation byte differences and shifted relocation offsets; the
   related `0x13e0` walk supplies a concrete re-entry discriminator. No unsuccessful source is
   promoted merely to increase a counter. Its handoff identifies a new source
   region and the next discriminator while compiler research continues.

The pilot establishes reproducibility, useful discrimination and stricter input
identity checks. It does not claim a strict-byte gain. Broader infrastructure
adoption should follow a measured target-family gain, not the number of variants.

The 2026-10-11 extension transfers controlled experiments and cross-module
evidence, using Hercules's own pinned RTM compiler. Three small actor-reset
controls isolate storage conversion from constant sharing, and separate fields
from an array. Their outputs distinguish hypotheses without solving the native
full-function register allocation. ENG1/ENG3 reset spans are byte-identical;
TITLE's related reset is not. The existing engine source now has two bounded
behavior fallback mappings, with explicit memory and cdecl preconditions.

The genuine historical SDK display pilot adds 816 complete masked code bytes
while preserving every strict match and cleanup caller contribution. It also
exposes real missing dependencies rather than forcing a link with placeholders.
Continue recovering those bodies in parallel with evidence-driven compiler
research. No new matching framework or relaxed acceptance is warranted by these
results. Current metrics, negative controls and next discriminators are in
`evidence/full_goal_display_reset_wave_2026_10_11.json`.

## Validation and continuation

Before changes: fresh **113 FUNCTION_MATCH instances / 7,840 bytes** and
**331 masked rows / 80,928 bytes**, all passing. The bytes include natural padding.
Legacy `CODEGEN_SIMILAR` contributes a separate 352 bytes; the one `RECONSTRUCTED`
row contributes 224 bytes. Finite semantic fixtures do not prove exhaustive
behavior, so do not relabel near-exact drafts as behavior-exact automatically.

Use the configured analysis Python (the Windows `python` alias is unavailable on
this host). Historical VC5 launching requires the existing process-only escalation
for its DLL layout; no toolchain binary was changed. Generated objects, listings,
maps, receipts and reports remain ignored.

```powershell
python scripts/probe_constant_lifetime.py
python scripts/function_map.py ENG1.DLL
python scripts/function_map.py ENG3.DLL
python scripts/convergence.py --function-map work/function_map/title_v4.json --function-map work/function_map/eng1.dll.json --function-map work/function_map/eng3.dll.json --output work/convergence/report.json
python -m unittest discover -s tests -p 'test_*.py'
python scripts/match.py verify --report work/convergence/raw_regression.json
python scripts/similar.py verify
python scripts/inventory.py --verify
```

Fresh final checks, tool/runtime provenance and pilot identities are recorded in
`evidence/blood2_transfer_pilot.json`. Do not use those historical measurements as
an acceptance cache. Resume indexed families with `region.py packet`; retain useful
ordinary source and immutable negative receipts. Reopen a stopped family only with
new evidence, changed context or a new discriminator, while the independent
missing-source lane continues.

The subsequent implementation increment is recorded in
`docs/title_data_progress.md`: 1,016 bytes of real initialized TITLE data and two
resolved dependencies, with existing function identities preserved. It also
corrects an unsupported data-ownership lead and retains the independent ENG3
pointer-anchor blocker.

The parallel follow-up in `docs/parallel_convergence_progress.md` resolves that
ENG3 pointer-anchor blocker with an independent accepted Hercules layout anchor
and a measured preincrement selection form: two new masked functions / 128 bytes.
It also records new TITLE graph/edge discriminators and a newer Blood II reference
snapshot. Compiler-specific conclusions and raw acceptance remain unchanged.

The resource-pointer pilot now distinguishes a real source-value graph from
ownership alone. With the actual copied interface fixed, direct member access
uses ECX for the pointer and EAX for zero; capturing that pointer in a local
uses EAX and ECX respectively and leaves ten nonrelocation differences. One
full six-service confirmation restores the already recovered preparation helper
and preserves all five peers byte-for-byte with ordered references. Three fresh
RTM compilations support this narrow cause; no general allocator rule, new strict
bytes or full-service equality follows. Reuse this experimental sequence only
where native single-use/reload evidence supports it. See
`evidence/resource_pointer_capture_control_2026_10_11.json`.

The accompanying host-table audit found a measurable strict pilot: a genuine
16-byte return-one body shared by six table entries, recovered with the existing
leaf and promotion gates. All 120 previous strict functions remain exact. The
48-source natural EXE build preserves all 47 older contributions and still has
15 real unresolved dependencies. This supports continued actual body/owner
recovery alongside bounded compiler experiments; it does not justify a new
framework or manufacture a complete table. See
`evidence/full_goal_pointer_callback_wave_2026_10_11.json`.
