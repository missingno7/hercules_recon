# Current handoff

Current source is consolidated under `src/{pc,title,eng1,eng3,shared}`.
Read [the source guide](../src/README.md) and [layout migration](source_layout.md).
Active build selection is in `config/`; historical paths in dated reports are provenance.

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
`c12c7fc` and the earlier `6640420` branch work. After the 2026-10-24 direct-call,
actor and engine-helper batches, the 2026-10-25 caller-entry review, and the first
loader-zero, collision-pointer, spatial-predicate, spatial-neighbor, target-steering,
initialized angle-table, motion/contact, grid-caller, scene-cell, WORD-limit and
pool-pressure, allocator/base-initializer, pool-caller, pointer-owner, pointer-client
and player-state-owner/program/animation, coordinate-factory, action-appender,
middle-factory, initialized boss/attachment-Entry and actor-pair animation batches, there are
487 accepted function rows at 487 distinct native entries / 49,024 bytes:
137 RAW rows, 43 retained linked rows and 307 RELOCATED rows / 34,816 bytes.
These batches add 152 functions / 22,512 bytes, with independently
reviewed references and four bounded local switch tables. See
`docs/nfsmw_matching_2026_10_24.md`, `docs/actor_call_followup_2026_10_24.md`,
`docs/engine_helper_matching_2026_10_24.md`, `docs/full_matching_2026_10_25.md`,
`docs/loader_zero_matching_2026_10_25.md`,
`docs/collision_owner_matching_2026_10_25.md`,
`docs/spatial_predicates_2026_10_25.md`,
`docs/spatial_neighbors_2026_10_25.md`,
`docs/spatial_queries_2026_10_25.md`,
`docs/angle_lookup_matching_2026_10_25.md`,
`docs/motion_contacts_2026_10_25.md`,
`docs/grid_callers_2026_10_25.md`,
`docs/scene_cell_matching_2026_10_25.md`,
`docs/pool_limit_alias_matching_2026_10_25.md`,
`docs/pool_pressure_matching_2026_10_25.md`,
`docs/allocator_matching_2026_10_25.md`,
`docs/pool_callers_matching_2026_10_25.md`,
`docs/owner_alias_matching_2026_10_25.md`,
`docs/pointer_clients_matching_2026_10_25.md`, and
`docs/player_cells_matching_2026_10_25.md`,
`docs/player_programs_matching_2026_10_25.md`, and
`docs/player_animation_matching_2026_10_25.md`, and
`docs/coordinate_factory_matching_2026_10_25.md`, and
`docs/action_appender_matching_2026_10_25.md`, and
`docs/middle_factory_matching_2026_10_25.md`, and
`docs/boss_entry_matching_2026_10_25.md`, and
`docs/attachment_entry_matching_2026_10_25.md`, and
`docs/actor_pair_animation_matching_2026_10_25.md`.
The RAW gate and complete relocated instruction/reference requirements remain.
Explicit, independently owned loader-zero references now have fail-closed support.
Separately, eleven missing player-mode bodies now have real source in
`src/eng1/player_modes_remaining.c`. Two linked rows retain
earlier raw proofs,
so `match.py verify --raw-only` checks 139 proofs without double-counting rows.
All 134 old RAW rows remain accepted; `exe.host_0a190` adds 16 RAW bytes and
`eng1.native_code_target_002ad70` adds 224 RAW bytes. The unconditional base
initializer adds 272 RAW bytes in a fully inventoried single-function unit.

The 2026-10-23 2,211-row / 502,791-byte source census left 1,667 unaccepted rows with
reference-only differences, 177 with code differences, 13 with extent differences,
and 19 raw-equal diagnostics awaiting unit ownership/inventory review. Those counts
are diagnostic categories, not additional matching acceptance or whole-game coverage.
Existing diagnostic rows account for 126 of the latest additions. Eighteen new or
corrected bodies were reconstructed and accepted: two axis bodies, one
player-mode body, two spatial predicates, five spatial neighbors, target steering
and the cosine-phase search, plus two motion/contact, two fixed-degree helpers
and the alternate-pool pressure helper, plus the corrected base initializer.
The nearest-actor query has real source and finite evidence but remains unaccepted
with compiler differences and unproved worklist ownership/capacity. Six player-mode functions
remain diagnostics; their unresolved references do not add accepted matches.

`leaf.py residual --baseline-object ...` now optionally adds bounded, pinned
objdiff candidate comparisons alongside the original-backed residual report.
See `docs/workflow.md`. The tested signed-address input initializer remains in
ignored candidates/: the existing independent-evidence manifest pins its canonical
source for earlier proofs. Mixed-unit raw adoption restrictions remain unchanged.

TITLE's 362 mapped source functions link. ENG1 has 962 compiled mapped providers
out of 1,231 diagnostic spans and 730 genuine unresolved symbols (435 function,
295 data/untyped); no generated placeholder is in that build. The latest angle
batch adds two actual ENG1 lookup providers and reuses the existing complete
initialized sine/tangent owner. All 3,522 isolated angle cases agree, including
one physical divide exception. Unique full-payload searches identify the tables
independently of caller addresses; six wrong-payload/base controls reject.
The subsequent four motion/contact and fixed-degree helpers pass 1,550 guarded
cases. Two stale diagnostic contact placeholders were retired and their physical
ABI aliases now target the real source. Contact services remain modeled in that
fixture; their scene/grid/resource chains are not proved complete. The existing
20b50 grid candidate is explicitly known incomplete, with unproved row/header/
stream capacities and four finite native cases. See its current source and
`evidence/eng1_grid_query_20b50.json` before treating entry identity as closure.
The later Unicorn-only grid composition expands that evidence to 725 native
cases / 1,450 comparisons of canonical source and a private draft, each with the
real stream decoder. Wrong reverse-seed and skipped one-past-cell mutants fail
with different return values. The draft reaches the observed 480-byte span but
still has 358 non-reference differences and is neither integrated nor accepted.
See `docs/grid_scan_composition_2026_10_25.md`; native row/header/stream capacities
and non-tag-zero decoder families remain unproved.
Seven unchanged grid/contact callers subsequently gained complete relocated
acceptance for 1,264 bytes, without transferring acceptance to their referenced
scene/grid helpers. The private 29870 scheduling control reaches 3 residual bytes
but also changes unrelated unit contributions and remains ignored; canonical
29870 still has 10 differences. No source-graph providers or closure changed.
The scene-insertion twins at ENG1 295a0 and ENG3 1e440 now also pass complete
relocated proofs against independently identified four-byte scene/support cells.
Their unchanged COMMON owner and 1,792 isolated comparisons prove only those
slots/functions. All three resetters and the shared normal initializer subsequently
gained 416 bytes of complete relocated acceptance. Their complete WORD owners
have independent native anchors. An explicit pinned compiler-renaming header
connects the existing legacy names to those owners; fresh whole-unit COFF
comparison permits only those undefined-data name changes. No gameplay or
storage source was changed. The 624 guarded resetter comparisons agree, with
cleanup/initialization services modeled. A HIGHLOW census accounts for all 34
direct root references; an inline jump table had hidden one WORD load from the
linear scan. The initial private `1b1d0` draft had 17 positional differences.
Resetting and reusing the scan cursor resolves them; its new real provider now
has complete 96-byte relocated acceptance. All 421 earlier accepted records
remain identical. The helper passes 382 guarded original/source cases, including
full-DWORD argument controls and a synthetic 65,535-slot bank. See
`docs/pool_limit_alias_matching_2026_10_25.md` and
`docs/pool_pressure_matching_2026_10_25.md`. The source build confirms the new
provider and the same unresolved-symbol set. `leaf.py packet` now decodes from
the requested entry instead of inheriting the database's linear alignment.
The two real allocator providers now pass complete relocated proofs for 656 bytes
against ten actual WORD owners and two pointer cells. The configuration writer
independently anchors the counters; all 76 overlapping HIGHLOW WORD accesses are
classified. The two shared diagnostics migrated without changing old source or
TITLE/ENG3 peer inventory. Their 1,922 guarded comparisons cover first-fit/wrap
ordering and show that success neither advances nor reserves a slot. The base
initializer now also has a real RAW-exact provider for 272 bytes, using the
existing recovered signed-index view and assignment groups. Its 198 guarded
cases preserve all 58 ordered stores and 18 untouched bytes. All 422 earlier
accepted records remain unchanged. See `docs/allocator_matching_2026_10_25.md`.
The final source build explicitly retains the old unit's remaining normal
initializer after its last recorded sibling migrated. It has 962 mapped
providers, the same 744 unresolved symbols, zero duplicates/placeholders and
78 preserved RAW functions; both new allocator object contributions were rechecked.
Shutdown and the normal/alternate startup sweeps subsequently gain 192 relocated
bytes with actual same-name scalar owners. Two additional middle cursor/start
WORDs have independent configuration anchors; no alias support was added. The
168 guarded comparisons compose real reset loops under shutdown and model their
lifecycle services, including live count/bank changes and final cursor-start
reloads. The middle startup sweep migrates to its existing real source but stays
diagnostic because indexed pointer-table ownership is unclosed. All 425 prior
accepted records remain unchanged. See `docs/pool_callers_matching_2026_10_25.md`.
Ten unchanged player/pickup/scene callers subsequently gain 1,120 relocated bytes
through explicit data-only pointer-owner proofs. The caller is compiled unchanged;
typed wrappers rename the two actual four-byte COMMON owners and must preserve
their complete objects. Twenty-nine new adversarial tests protect that distinct
mode, including historical compiler type checks and publication races. All 428
prior accepted records remain identical. See `docs/owner_alias_matching_2026_10_25.md`.
Four additional unchanged clamp/animation/sine clients gain 720 relocated bytes.
They reuse both pointer owners and the complete initialized sine payload. Their
2,117 guarded comparisons compose the real frame selector and model the other
services, including root/counter changes that test state reloads. All 438 prior
accepted records remain identical. See `docs/pointer_clients_matching_2026_10_25.md`.
Five complete player-state cells subsequently gain actual source owners, closing
exactly five unresolved data symbols. Their 128 original relocation-root operands
all have direct exact-root DWORD accesses; independent setup/burst/cruise writers
anchor three opaque pointers and two signed DWORDs. Twenty-six unchanged callers
gain 2,576 relocated bytes. Their 2,418 guarded cases include 3,066 invocations
and 324 composed flag/scroll sequences, with real frame helpers and modeled
remaining services. All 442 previous accepted records remain identical. See
`docs/player_cells_matching_2026_10_25.md` and its frozen evidence.
The existing mirror DWORD and two existing flag programs subsequently close
416 relocated bytes in four unchanged setup callers. Ten arrays / 186 bytes now
have a bounded frame/sound/restart closure proof, with all words observed during
3,697 real interpreter invocations in 453 guarded scenarios. Sound and signed
pairs remain modeled; real initializers and frame helpers execute. The pending
caller independence guard first rejected old burst anchors inside3aef0. A separate
anchor-only transaction recompiled all 427 prior strict proofs before and after,
preserved every function identity and immutable parent manifest, and retained
old proofs for 26 manifest members while replacing 20 instruction anchors.
No source, cell destination/owner, profile or verifier policy changed. See
`docs/player_programs_matching_2026_10_25.md`.
Three additional unique animation objects / 92 bytes now have complete typed
pair/WORD owners. They close exactly three unresolved symbols. The unchanged
37390 return-transition client gains 112 relocated bytes. Its 306 guarded cases /
1,103 top-level calls compose both real interpreters and reset, including an
actual change from the longer WORD loop to634e0. Removing unrelated C++ bodies
changes200 pair-interpreter bytes, so that fixture retains the canonical unit
unchanged and denies unrelated entry/read paths. All 472 prior accepted records
remain identical, including reference histories. See
`docs/player_animation_matching_2026_10_25.md`.
The two coordinate factories now use an ENG1 C++ provider with the existing
DWORD address owners at71388/713a4. Independent configuration stores, complete
four-byte COMMON owners and18 direct-root references close their data bindings;
all four helper entries have earlier freshly rechecked proofs. The old4/2 SIB
residual describes an older source context: both current C and shared C++ were
already instruction-equal. There was no new expression sweep. Their264 guarded
native/source cases compose the actual allocator and complete initializer
chain, including explicit external descriptor-root interventions. All473 prior
accepted records and242 other integrated source objects remain identical. See
`docs/coordinate_factory_matching_2026_10_25.md`.
The two action appenders now use actual DWORD callback roots and the WORD limit.
The separate explicit rooted-callback proof verifies exact source/native
load/mask/stride/argument/call/cleanup provenance and an independent native
witness outside all pending callers. Default unreviewed indirect calls still
reject. All 434 prior strict proofs retain identical instruction/reference/context
hashes across the verifier addition, and 20 new adversarial tests pass. The
776 composed cases execute real factories/allocators/initializers with632 modeled
callback mutations, including signed count wrap and16 group/actor aliases. The
retirement neighbor moves source but remains diagnostic because its indexed
registry extent is unproved. All475 earlier accepted records remain identical.
See `docs/action_appender_matching_2026_10_25.md`.
The middle allocator275d0 and coordinate factory277e0 now have a real provider
with complete336/128-byte native contributions. Five existing WORD owners, the
middle pool pointer and its descriptor DWORD have independent native population
anchors. Their1,111 guarded cases /1,411 top-level calls compose the actual
middle/base initializers, distinguish non-reserving allocation from creation,
and cover descriptor aliases, root changes and large readable pools. All477
previously accepted records remain identical. Source-plan omissions replace the
older352/176-byte runtime bodies while preserving their six remaining functions;
241 other source objects remain identical. See
`docs/middle_factory_matching_2026_10_25.md`.
All 438 strict proofs, 43 linked function checks, 321 tests and 1,556 supported
masked rows across 199 sources pass; 86 existing broader linked provenance drifts remain.
The subsequent worklist correction adds no matches: normal asset reads now
reload the record kind, while its callback retains the entry kind; all three
families reload asset roots between stores. The final Unicorn harness reproduces
54 prior mismatches and passes all108 corrected cases, with nine actual
source-prefix controls preserved. The standard worklist differential command
now uses this isolated path. Compiler equality, indexed group-owner extent and
dynamic dispatch remain open for these three functions. See
`docs/worklist_live_reads_2026_10_25.md`.
A subsequent private typed recipe reduces those contributions to the native
272/192/144-byte extents. Alternate has zero masked residual bytes;
normal/middle retain 2/17. All 108 cases and nine production guard controls pass.
`scripts/worklist_typed_probe.py` replays the frozen recipe without changing
providers or acceptance. Further type-only controls reproduced the same code;
review `docs/worklist_typed_probe_2026_10_25.md` before retrying them. Indexed
group-owner and callback proofs remain open even for alternate.
The next frontend control closes normal's two residual bytes by compiling the
same typed logic as C, and reduces middle to three SIB bytes. C++ still closes
alternate. All 108 C-profile cases pass with unchanged native results; the probe
accepts an explicit frozen C recipe. Four addressing controls produce identical
code. The 120 selected native HIGHLOW references across ENG1/ENG3/TITLE do not
establish the indexed owner's full capacity. See
`docs/worklist_c_frontend_2026_10_25.md` before another expression/type retry.
The initialized boss Entry owner then resolves `_g_entries_5a450` and closes
the unchanged25580 caller's two references. All240 cases execute the actual RAW
helper, and all244 earlier source units retain their complete contributions.
The new data-only unit reduces unresolved symbols to735. All439 strict proofs,
43 linked checks, 321 tests and remaining supported masked rows pass. See
`docs/boss_entry_matching_2026_10_25.md`.
The attachment follow-up adds four more complete Entry owners and accepts
3a730/3a8a0/3a910, then moves 33e10 to a real provider using WORD+60 of the existing
192-byte owner. All 480 previous accepted records remain identical. The old
calibration source and its thirteen accepted peers stay unchanged; only the
derived runtime body is omitted. All 1,200 composed cases pass, with a modeled
WORD interpreter only in the fourth caller. The graph has 247 units and 730
unresolved symbols; all 443 strict proofs, 43 linked checks, 321 tests and 1,551
supported masked rows pass. See `docs/attachment_entry_matching_2026_10_25.md`.
Eleven missing player-mode entries were implemented and removed from the diagnostic placeholder
manifest. Two neighboring axis-motion services now also have real source. All
thirteen bodies pass 470 isolated/composed original/source cases, with remaining
external services modeled. Nine have complete instruction equality apart from
unresolved references; four retain compiler differences. The input WORD view
reduces `19480` from 232 to 22 differing non-reference bytes. See
`evidence/eng1_player_modes_2026_10_25.json` for the earlier snapshot and
`evidence/player_axis_recovery_2026_10_25.json` for that snapshot. The two axis
bodies are now accepted relocated matches. Separate reference-only boundary
reconstructions extend isolated/composed testing to 565 cases; their unreviewed
header/table capacities keep them out of production integration and matching.
Nineteen measured
setup-dispatch bindings closed real dependencies. Ninety-nine other implemented
functions were removed from stale provisional classifications. ENG3 and the host
still need complete reviewed link plans; current source rows are 555 and 403.

The active goal is full faithful first-party function matching under the existing
RAW/RELOCATED rules, with every earlier proof preserved. All five indexed actor-pool
entries now have function proofs, though the shared normal initializer's runtime
provider remains a separate concern. Use the independently audited WORD owners
to review further actual callers; preserve the frozen headers/manifests.
Literal pointer-name renaming changes functions in all three tested caller
units. The distinct `owner-compile-renaming` mode instead keeps the caller
unchanged and proves one complete four-byte pointer owner with a typed `/WX`
wrapper and exact full data-only COFF comparison. The old WORD mode is unchanged.
See `evidence/owner_alias_progress_2026_10_25.json` for that mode's controls and
`evidence/pointer_clients_progress_2026_10_25.json` for that caller batch.
The current matching/source-graph census is
`evidence/attachment_entry_progress_2026_10_25.json`. The private worklist compiler
investigation remains `evidence/worklist_c_frontend_progress_2026_10_25.json`.
New reference work should start from the smallest complete independently owned
cell or helper entry; do not infer ownership/capacity from operand agreement.
Player glow/trail pointers at 0x707dc/0x707fc, burst-origin pointer at 0x707e4,
cruise DWORD at 0x707e8 and flag DWORD at 0x707f8 now have complete canonical
owners in `player_state_cells.c`. Preserve that header and the accepted bindings.
The burst initializer now uses the existing mirror DWORD owner at 0x734f4 with
an independent native writer. Two setup programs have complete closed-loop
proofs from their actual WORD consumer, independently of adjacent labels. The
returning program at0x634e0 now has its complete nine-WORD owner, as do the
38-byte loop at634b0 and 36-byte pair loop at633e8. Payloads at63350/63378 are
identical and the six-byte634d8 payload occurs seven times; independent physical
ownership is still needed for them. Do not infer extents from the next label or
promote all historical prefixes because some have proved complete for a consumer.
The
normal/alternate factory source still has four/two scale-1 SIB differences: the
existing negative-knowledge record requires a new source-abstraction/TU
discriminator before retuning equivalent pointer expressions. Their descriptor
pointer identities/complete owners also need review. The final
`allocator_progress_2026_10_25.json` records the provider migrations and limits.
Also resolve player-mode compiler differences at `0x19260`, `0x19290`, `0x193f0`, and `0x19480`, then
the neighboring image/data and missing service dependencies. Keep the new unit
editable while its compiler context is being recovered. For matching,
verify callback-table/BSS ownership before expanding symbolic-reference support;
do not retune reference-only functions. One pool-pointer scalar now has a fresh
independent owner/slot audit in `evidence/normal_pool_pointer_2026_10_25.json`.
That audit supplies independent identity evidence for the new explicit COMMON-owner
path; the old file-backed `data` path still rejects this storage. Three caller
functions now pass the full relocated gate with that owner. Next audit other
named scalar owners independently. Explicit complete-WORD compiler renaming now
has its own guarded proof path, as does the separate complete pointer-cell owner
mode. Pointer views, COMMON merging and indexed BSS remain unsupported. Preserve
both accepted alias modes' headers and frozen bindings.
See `docs/loader_zero_matching_2026_10_25.md`,
`docs/player_axis_recovery_2026_10_25.md`, and
`evidence/realignment.json` for the
frozen commands, counts and limitations of this consolidation.
