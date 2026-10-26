# Everyday reconstruction workflow

Start with the [project direction](project_direction.md), current recovery state,
and the focused source/evidence for the function or dependency family. Work from
a falsifiable question; keep experiments under ignored candidates/ or work/.
Generated packets and reports are disposable and never grant acceptance.

To choose lanes, rank the reference blockers of all pending instruction-equal rows
against the current ledger and accepted binding documents (about 180 fresh compiles,
planning only; implied destinations never establish identity):

    python scripts/blocker_queue.py --census work/realignment/census/recorded.json --output work/blockers/queue.json

Cut lanes by blocking symbol and lane type (identity reuse, file-backed data, scalar
loader-zero owner, callee identity); see [velocity_b](matching_velocity_2026_10_26b.md),
which accepted 406 functions this way, including the `fixed-slot-v1` interface profile.

For the earlier blocker frontier and measured integration results, see
[the velocity pilot](matching_velocity_2026_10_25.md). Build a planning view from
the recorded census and small, reviewed handoffs; do not rebuild the census:

    python scripts/region.py matching-frontier --census work/realignment/census/recorded.json --handoff work/lane/handoff.json --ids title.0cce0 title.0cd00 --limit 8

Handoffs name `function_ids`, `source`, `source_sha256`, ordered `flags`, `target`,
`target_sha256`, `outcome`, and `blockers`. Each blocker names affected IDs, a typed
`requirement`, and hashed `evidence_files` (`path`/`sha256`). Group an owner only
when the requirement, module identity, and evidence agree; an address or name is
insufficient. The frontier filters accepted/stale work and reports exact-context
negative knowledge. Old census context remains `recorded_only`; every candidate
requires current preflight. Unclassified proof stays local rather than acquiring
an inferred shared identity. Truncated output reports omitted task IDs.

For unchanged-unit RAW candidates, check eligibility before integration:

    python scripts/match.py preflight --raw --select module.function --objects work/lane/objects.json --report work/lane/preflight.json

Add `--preserve-reconstructed` only for the existing reviewed mixed-status path.
Reference preflight also accepts explicit `--select` IDs. These checks inventory
recorded contributions and current inputs without compiling; they never publish
acceptance. Promotion now rejects unsupported unchanged-unit metadata before
running full protection. Fresh compilation, peer protection, independent reference
proof, and journaled publication remain required.

For complete invocation accounting in a new campaign, set
`HERCULES_COMPILE_METRICS_DIR=work/<campaign>/compiler-events` in its process
environment. The existing compiler wrapper writes one uniquely named JSON event
per attempt, including UTC bounds, duration, command, source hash, compiler identity
and exit/error. It also captures fixture calls when their temporary root is later
removed. The path must resolve within this repository's ignored work/build trees.
Unset it to use the normal path. Events are diagnostic, contain no acceptance
cache, and measure compiler execution rather than worker or integration wait.

The next [24-worker campaign](matching_velocity_2026_10_26.md) accepted nine
functions / 688 bytes and closed six ENG1 dependency names. Keep handoff metadata
outside reference function records: each `functions[id]` binding permits only
`symbols`, `tables` and `indirect_calls`. The comparator's existing schema guard
now also runs during evidence loading, before compilation and in preflight.
Use existing reference commands/API receipts rather than a custom lookalike that
omits include context or mixes a recovery row into a binding.

For private candidates, resolve includes with `compile_inputs` before spending a
compiler call; preserve the source language and canonical include roots. Inspect
actual callee declarations before importing a broad region header. An opaque
native pointer type and evidenced field views can avoid incompatible inherited
prototypes without changing a shared header. Verify the complete contribution.
Track saved-register stack movement before inferring argument positions.

For ENG1 active ownership, consult the source graph's `compiled_sources` and
compiled definitions. `functions[].additional_evidence` can contain historical
source paths; it is not the build selection. Reuse reviewed source-plan omission
and declaration adaptations when reconciling selected providers, preserving the
historical unit's proof context and one production implementation.

For a genuine code-generation residue, the bounded
[cpp_permuter pilot](permuter_pilot_2026_10_25.md) establishes an optional explicit
alternative generator with the pinned VC5 toolchain. It produced two
instruction-equal candidates and no new STRICT acceptance; finish their reference
proof before reopening those source searches. Use existing comparison gates,
reviewed alternatives and hard caps; avoid its random/all-pass defaults.

## Short matching campaign loop

Before spending compiler calls, review a focused source/region packet, exact
function IDs, current source ownership and hashes, original callers/callees, and
only the few relevant negative notes. For a proposed lane, run the compiler-free
preflight against its canonical bindings and recorded baseline objects:

    python scripts/match.py preflight --bindings evidence/lane.json --objects work/baseline.json --report work/lane/preflight.json

Preflight is a diagnostic, not a match or acceptance proof. Follow it with the
focused fresh candidate/reference check and the normal fresh promotion gate; do
not reuse preflight or prior-run proofs. Run at most one fresh compile per distinct
compiler profile for each experiment. Programmatic probes should use the pinned
`match.compile_source` helper and write generated source as UTF-8; avoid ad hoc
compiler environments or flag overrides.

Use tens of narrow Luna workers with one integrator: start with 24 on this host,
not a four-to-six-worker ceiling. Parallelize LLM inspection, source recovery and
reference proof across disjoint units. Each worker gets exact IDs, one objective,
an explicit compile/token budget, source/symbol ownership, a focused existing
packet and a required verification method. No full-history fork or repeated
repository census is needed. Give one Sol investigator a shared causal blocker
when it could unlock several lanes; routine work stays with Luna.

Keep workers productive while the integrator consumes ready batches. A batch need
not wait for the slowest worker; completed and blocked workers get a different
independent task. Limit each handoff to the candidate, existing-format proof,
input hashes, focused result and first blocker. Reuse a diagnostic baseline only
after checking its source/include/flag/object identities. A new unrelated commit
does not invalidate unchanged inputs, but a changed shared header does. Workers
never write the ledger/shared headers, run the full gate, or start more agents on
their own. Use the existing native subagent tools, not a new routing framework.
Use `followup_task` for another assignment to a completed worker; `send_message`
does not restart one. Notify running workers without creating overlapping tasks.

The host exposes 32 logical processors and about 63 GiB to the runtime. An ignored
capacity probe freshly compiled an existing RAW control 8 and then 24 ways in
parallel: all 32 complete-function checks passed, every object path was unique,
and all timestamp-independent unit inventories agreed. Measured throughput was
13.38 and 14.12 compilations/second respectively; this is an isolation check, not
evidence that tripling compiler concurrency triples throughput. Extra LLM workers
primarily parallelize reasoning. Receipt: `work/scale/compile_capacity.json`.

Rerun the full source graph only after source, header, or active-selection inputs
change. The integrator runs fresh full protection once at each actual batched
integration boundary; never carry strict proofs across runs. Keep the number of
ready, independently verifiable candidates small enough to review promptly; if
that queue grows, spend workers on missing evidence or other families rather than
producing more unreviewed variants.

Keep the handoff small: exact IDs and source ownership, base revision and current
source/input hashes, existing-format bindings plus native evidence, one focused
receipt, compiler-call count, and first blocker. Link reports in the workspace;
do not paste whole ledgers, reports, or hash lists into chat. Routine commit
reports need only generated statistics, exceptions, and the next four to six items.

The 2026-10-25 process check preserved all 496 strict proof hashes while replaying
eight functions from two binding documents, with 43 legacy LINKED checks passing
(their 86 existing provenance drifts remain). The fresh strict phase used 122
compiler calls both times: 84.237 seconds before, 60.148 after. This single timing
pair is not a performance guarantee. The 373-test suite passed; preflight also
rejected a deliberately wrong relocation offset without compiling. Recovery state
and game source stayed unchanged. Scratch receipts are under `work/process/`.

For a bounded matching campaign, use the measured
[census, worker loop and pilot results](matching_realignment_2026_10_25.md).
Classify the first current blocker before compiling a source variant. The recorded
census now retains diagnostic object paths, complete hashes and commands; inspect
those objects instead of rebuilding a baseline in every worker. A fresh promotion
gate is still mandatory. Keep one writer for source selection, bindings and ledger
publication, and count full regression/module builds separately from focused work.

Reuse existing recovered source and cross-module twins before retuning a body.
A recorded residual belongs to its source hash and compile profile. Freshly
check the current provider and existing twins before treating an old stop note
as a current mismatch. Both coordinate factories already had instruction-equal
current sources; independent owner/reference review then closed their matching
obligations without another expression sweep. See the
[factory review](coordinate_factory_matching_2026_10_25.md).
A correct signed field view and existing assignment groups can explain compiler
constant lifetimes; verify the view from native consumers and compare ordered
stores, not only final memory. A shared diagnostic can migrate to a real module
provider only with its old record retained, unchanged old-unit inventory and
fresh checks. That migration grants no acceptance. See the
[allocator/initializer recovery](allocator_matching_2026_10_25.md); it uses the
existing RAW and RELOCATED gates without changing their policies.

After moving a source unit's last ledger row, check whether the build still
selects its other real providers. Keep a needed historical unit explicitly in
the source plan, with replaced bodies omitted. Rebuild and compare actual
unresolved symbols and provider ownership; matching proofs alone cannot catch
that dependency-selection regression.

When replacing a runtime body in a retained unit, first try the exact planned
source omission privately. Compare every remaining complete function and account
for retired noncode storage. The middle allocator/factory migration preserves
six remaining contributions; only two guard strings and the retired bodies'
frame records disappear. See the
[middle-family review](middle_factory_matching_2026_10_25.md). Its differential
also distinguishes repeated allocation from creation: only creation reserves
the slot by storing a nonzero kind.

For initialized-data references, first look for an existing complete source
owner. Compile it freshly and compare its entire unrelocated payload against
file-backed native storage, independently of the caller's implied addresses.
Require a unique complete occurrence and retain wrong-payload/wrong-base
controls. This can close several modules without new storage guesses; see
[the angle-table audit](angle_lookup_matching_2026_10_25.md). Shared matching
rows still need actual source providers and reviewed aliases for integration.

For the bounded `20b50` grid composition, run
`python scripts/eng1_grid_scan_differential.py`. It executes original/source
instructions in Unicorn with the real stream decoder and synthetic readable
storage, checks row-read and decoder-call traces, and never loads an original
OS module. See [the composition scope](grid_scan_composition_2026_10_25.md).
Its fixtures do not establish native allocation capacities or matching acceptance.

For the pointer clamp/animation/sine clients, run
`python scripts/pointer_clients_differential.py`. Its private source extraction
must preserve every selected complete canonical COFF contribution before linking.
It composes the real frame selector, models the other services, and compares
ordered stores, root/table reads and call order under changing roots/counters.
See [the caller review](pointer_clients_matching_2026_10_25.md). Finite cases do
not establish the modeled helpers' behavior or native actor capacities.

For player-state owners and their clients, run
`python scripts/player_cells_differential.py`. It composes the real frame helpers
and empty entry, with explicit models for the remaining services. Three-step
flag/scroll sequences keep state between calls; guarded cases compare cell reads,
ordered stores, complete synthetic memory and call arguments. Its private
extraction must preserve every complete canonical contribution before linking.
See [the player-state review](player_cells_matching_2026_10_25.md). A direct-root
operand census does not exhaust computed aliases or prove actor capacities.

If a pending caller contains an identity anchor used by already accepted BSS
references, preserve the independence check. Review a separate native anchor
and the exact affected accepted-row inventory, retaining the same cell RVA,
extent, owner, aliases and relocation uses. `scripts/reanchor.py --review
evidence/review.json --report work/reanchor.json` freshly verifies all strict
proofs before and after, preserves original manifests and prior proofs, and
journals the reference-metadata update. It grants no new acceptance. See
[the program/anchor review](player_programs_matching_2026_10_25.md).

For the additional player pair/WORD animation owners, run
`python scripts/player_animation_differential.py`. The pair profile uses fixed
four-byte records and a cursor restart to -1, rather than the WORD format's
duration header and restart to zero. It runs both real interpreters and the
return-transition client. The pair interpreter's measured context sensitivity
requires retaining its canonical C++ unit; unreachable fixture link definitions
remain guarded and never enter production. See
[the animation review](player_animation_matching_2026_10_25.md).

## Inspect and compare a candidate

For rooted action callbacks, the ordinary scalar owner proof alone is
insufficient. The explicit `rooted-callback-v1` profile additionally checks exact
dispatch provenance and an independent native witness outside every pending
caller. `python scripts/action_appenders_differential.py` separately composes
the real factories/allocators/initializers with modeled callback mutations,
including count wrap and count/actor alias cases. Keep dynamic callback behavior
and table capacities outside those claims. See the
[appender review](action_appender_matching_2026_10_25.md).

Worklist factories have different kind lifetimes: normal asset reads reload the
record kind, while its callback uses the entry kind; middle/alternate use the
cached low12-bit kind. All reload their asset roots between stores. The standard
`python scripts/eng1_worklist_factory_differential.py` now uses Unicorn plus nine
source-only prefix-guard controls. Its record aliases and root/kind interventions
exposed 54 previous source mismatches. See the
[live-read review](worklist_live_reads_2026_10_25.md). Finite semantic agreement
does not resolve the compiler, indexed-owner or dynamic-dispatch proofs.

For a reproducible private typed compiler control, run
`python scripts/worklist_typed_probe.py --output work/worklist_typed_replay`.
It materializes a hash-checked recipe under ignored work/, freshly compiles,
and replays the same 108 native/source cases. Alternate now has zero masked
residual bytes; normal/middle retain 2/17. This does not replace the guarded
providers or close their indexed-owner/callback proofs. See the
[typed compiler review](worklist_typed_probe_2026_10_25.md) before reopening
already measured field-type or flag-expression controls.

Before reopening those expressions, also replay the unchanged logic through the
C frontend with `--recipe evidence/worklist_c_frontend_recipe_2026_10_25.json`.
That profile closes normal's final two residual bytes and leaves three SIB bytes
in middle/alternate; the C++ profile still closes alternate. Check reproducing
frontend context before another expression search. These profiles do not establish
historical source language or reference ownership. See the
[frontend/owner review](worklist_c_frontend_2026_10_25.md).

For a missing initialized Entry table, establish the record stride, tag offset
and first terminator from the real consumer, then compile the complete typed
owner and locate its entire payload uniquely in file-backed native storage.
Include the full terminal record and retain wrong-value/truncation controls.
`python scripts/boss_entries_differential.py` replays the new 576-byte owner and
240 cases with the real RAW helper. Its source-graph audit resolves one actual
data dependency and preserves every previous object contribution. See the
[Entry owner review](boss_entry_matching_2026_10_25.md).

Before defining a missing address-labelled data root, check existing complete
owners for an interior reference. `33e10` now uses WORD+60 of the actual 192-byte
owner rather than duplicate storage. A separate reviewed provider preserves the
old calibration source and its thirteen accepted peers; the runtime plan omits
only the replaced body. Compare every remaining contribution and account for
compiler-private static numbering with its complete owner and all uses. The
[attachment-family review](attachment_entry_matching_2026_10_25.md) records four
accepted callers, four complete Entry owners and five resolved dependencies.

When the function map is indexed, make a compact context packet:

    python scripts/region.py packet <region>

Use --more only when shared context is needed. For a bounded source hypothesis,
compile against the original function with the current raw candidate command:

    python scripts/match.py candidate <id> --source candidates/<candidate>.c

This performs a fresh pinned-toolchain build. Inspect the complete contribution
and every mismatch; a prebuilt object is useful for diagnosis but cannot promote
recovery state. Keep a prediction and a falsifier with the experiment.

`leaf.py packet MODULE RVA SIZE` decodes the requested entry afresh. The shared
linear database can lose alignment after inline jump tables; it remains useful
for locating inbound references and caller context. Packets report an undecoded
tail but do not classify inline data or establish function extents. See the
[alternate-pool helper](pool_pressure_matching_2026_10_25.md).

For a bounded original-backed mismatch explanation, use `scripts/leaf.py residual`.
An optional previous candidate object adds a compact objdiff comparison without
replacing the original comparison:

    python scripts/leaf.py residual MODULE RVA SIZE SYMBOL SOURCE --baseline-object work/previous.obj --report work/residual.json

This uses the unmodified objdiff-cli 3.7.0 at
`C:\tools\objdiff-3.7.0\objdiff-cli.exe`, SHA-256
`91f473c5753c7593253b53a9dbda839bb4d395fdabf449a7250940d3c4de05b4`, from
the [official Windows release](https://github.com/encounter/objdiff/releases/tag/v3.7.0).
It is optional and never downloaded implicitly. Reference differences are enabled;
output retains complete COFF sizes separately from inferred function-body sizes.
Percentages, candidate-object equality, and objdiff's zero exit status do not
grant acceptance. Reports are bounded and stay under ignored work/ or build/.

## Integrate raw-accepted source

Review source and original context, then use the existing journaled promotion
path for a function that meets the raw gate:

    python scripts/promote.py <id> --source candidates/<candidate>.c

Promotion protects established matches and checks the selected source unit.
Inspect the source before integration: exact bytes are not permission to insert
copied code, original-binary dispatch, unsupported assembly, or guessed behavior.
Recover any interrupted promotion before starting another one.

For raw-equal functions already in a fully inventoried canonical unit, use
`python scripts/promote.py <ids...> --existing-unit --report work/batch.json`.
This status-only path keeps source in place and freshly rechecks RAW, RELOCATED
and legacy linked peers. Add `--verify-only` for a full dry run when needed;
publication already performs the same fresh gate, so a routine reviewed batch
does not need both commands.

For an unchanged, source-only unit with previously recorded size-exact
`RECONSTRUCTED` peers, reviewed `--preserve-reconstructed` additionally checks
their full compiled hashes, extents, source, flags and toolchain. Their status
and proof remain unchanged. Only selected RAW-equal functions are accepted, with
the entire unit inventory and context pinned. Unpinned headers, changed peers
and missing/extra symbols reject publication. This is peer preservation, not
matching credit for the reconstructed neighbors; the default gate still refuses
unsupported peers. Review current unit metadata/inventory before the expensive
global gate, using the worker's hash-validated object as a diagnostic only.

The first 24-lane wave, its six accepted functions, costs and shared next blockers
are recorded in [parallel matching results](parallel_matching_2026_10_25.md).

## Revalidate and inspect source closure

Run the RAW and RELOCATED regression gate after canonical source changes:

    python scripts/match.py verify --report work/strict_verification.json

For a native-backed correction to unaccepted functions in a source unit that
already has relocated peers, use the bounded [source-context refresh](context_refresh_2026_10_25.md).
It requires an exact source/changed-function review and fresh baseline/final
strict proofs. No accepted bytes, binding maps, categories or targets may change.
RAW peers, header changes and new/removed functions are unsupported by that path.

Use the module's focused source-graph and differential entry points to expose
missing providers and test semantic claims. For example, TITLE and ENG1 have
their own documentation and source-graph commands. A successful link establishes
closure only for the inputs that were selected. Diagnostic placeholders do not
count as reconstructed gameplay and must stay out of production builds.

For existing source with exact code and reviewed reference identities:

    python scripts/match.py references <id> --bindings evidence/relocated_reference_bindings.json

The protected batch form accepts repeatable worker-specific canonical binding
files, so keep each lane's evidence in its own document rather than building a
large combined manifest:

    python scripts/promote.py <ids...> --relocated --bindings evidence/lane-a.json --bindings evidence/lane-b.json --report work/lane/promotion.json

Review the independent bindings before promotion. Add `--verify-only` for a full
dry run when needed; a routine integration need not run it before promotion,
which performs the same fresh protection itself. Source stays in its real
translation unit and every accepted peer is protected. Masked equality and implied
addresses alone cannot supply bindings.

Shared-source symbol names can retain addresses from another module. In the
2026-10-24 helper review, `_title_0c7e0` in an ENG1 contribution did not identify
ENG1 RVA `0xc7e0`; the full reference gate rejected that otherwise instruction-equal
hypothesis. Ground aliases in the helper's own earlier body/entry evidence and
check original entry witnesses outside the selected callers. Copied-interface
aliases also need their actual slot and ABI reviewed; ENG3's limited RNG bridge
uses a call/cleanup sequence while its raw RNG bridge is a tail jump. See
[the helper review](engine_helper_matching_2026_10_24.md).

For loader-zero globals, first audit the complete source owner and an independent
native population/read witness. The explicit `bss` record then supplies a pinned
source/flags/symbol profile and a native instruction/hash/operand identity.
Verification freshly compiles the owner alongside callers and preserves the same
complete instruction/reference gate. A source COMMON prefix or zero-filled native
range alone is insufficient. The pool-pointer audit and first three accepted
callers are documented in [the loader-zero review](loader_zero_matching_2026_10_25.md).
Pin the owner's complete include context as well as its source. Native clients
of an already verified initialized owner can reveal genuinely missing source;
the [actor-pair recovery](actor_pair_animation_matching_2026_10_25.md) reuses
one WORD owner for three real providers without duplicating storage.

For a legacy name of an existing WORD owner, use an explicit compiler-renaming
header only after the independent scalar identity/extent audit. The verifier
recompiles the whole caller unit through that pinned header and rejects any
change beyond the declared undefined-data names, including other functions,
unclassified executable storage and linker directives. Keep accepted headers
immutable; additional alias families need their own reviewed adapters and proofs.
See [the pool-limit proof](pool_limit_alias_matching_2026_10_25.md). That WORD mode
does not enable pointer aliases, COMMON merging or indexed BSS.

For a complete pointer cell in a C data-only owner, the separate explicit
`owner-compile-renaming` proof keeps the caller unchanged and renames the owner
in an ignored typed wrapper. Its exact inverse-only header and pointer kind are
pinned; a fresh `/WX` compile checks the declaration, and complete COMMON/section/
symbol inventory equality forbids merging or additional storage. Use this only
after independently auditing the actual cell/owner. The original native and
publication gates still apply; it supplies no pointer view or indexed capacity.
See [the owner-alias review](owner_alias_matching_2026_10_25.md).

Cross-check a linear memory-access inventory against original HIGHLOW fields
naming each audited root. Inline jump tables can desynchronize linear decoding;
decode unmatched fields from independently indexed function entries and account
for each operand's width. Keep supplemental census findings separate from frozen
accepted manifests. This still does not exhaust computed or adjacent-base aliases.

The ordinary module/build and diagnostic commands are:

    python scripts/linked.py build TITLE.DLL
    python scripts/eng1_behavioral.py --output work/eng1_behavioral/build
    python scripts/linked.py verify
    python scripts/match.py verify --raw-only
    python scripts/similar.py verify
    python scripts/census.py --recorded --jobs 2 --output work/census/recorded.json
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
