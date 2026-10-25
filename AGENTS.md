# Working contract

Reconstruct one faithful native source game from Disney's 1997 Windows Hercules.
The long-term runtime uses SDL3 and modern compilers, without original first-party
executable modules. Historical compiler matching, semantic recovery, renderer
evidence, and runtime experiments all contribute to that game. The PC binaries
are the evidence oracle; PSX builds are secondary.

## Proof and ownership boundaries

- recovery.json is the sole authority for accepted function matching.
  Source coverage, semantic evidence, dependency closure, and runtime completeness
  are distinct dimensions.
- Existing FUNCTION_MATCH rows keep strict RAW acceptance; FUNCTION_MATCH is
  their legacy ledger name. FUNCTION_MATCH_RELOCATED requires exact
  compiler-generated instructions and independently verified references. Masked
  similarity alone never qualifies.
- renderer_contract owns evidence-backed, GPU-independent original interfaces
  and reference utilities. Correct uncertain semantics from original evidence.
- dropin_renderer owns modern build, forwarding, diagnostics, and backend code.
  Consume renderer_contract directly. Treat this as an experiment, not a promised
  final architecture.
- Preserve original gameplay semantics: call ordering, state reloads, integer
  behavior, update order, RNG, resource behavior, and renderer command semantics.
- Original host execution still requires explicit authorization, verified write
  and profile isolation, windowed startup, and display restoration. A synthetic
  fixture or pre-entry probe does not satisfy that gate.
- Screenshots, forwarding experiments, and finite comparisons prove only the
  boundary they measure.

## Historical source and verification

- Keep current game source under src/pc, src/title, src/eng1, src/eng3 or
  src/shared, according to ownership. Matching status is per function in
  recovery.json; do not split source into behavioral, matching or calibration
  tiers. Active source/link selection belongs in config/. Preserve real compiler
  units; see src/README.md and docs/source_layout.md for the reviewed migration.

- Never modify assets/. Never commit original or extracted game files, tools,
  generated builds, databases, or large disassemblies.
- Keep experiments in ignored candidates/ or work/. Reuse existing source and
  evidence; do not clone full source trees for experiments.
- Fresh compilation with the pinned historical toolchain and complete target-byte
  equality are required for the existing FUNCTION_MATCH gate.
- Do not copy original machine code, patch instructions, dispatch into original
  binaries, use byte-array functions, or substitute guessed gameplay stubs.
  Diagnostic fail-on-entry placeholders must not enter production builds.
- Document any measured compiler steering and preserve the source logic. Original
  function addresses, ordering, padding, PE placement, and whole-image equality
  are not matching milestones.
- Protect all accepted raw and relocated matches after canonical source changes. Identify
  compiler and runtime library code instead of casually rewriting it.
- Keep claims narrower than their evidence. Function equality does not prove
  source identity, semantic completeness, module layout, or whole-image equality.

## Work coordination

- Use main as the canonical integration branch for the one reconstruction.
  Keep task branches temporary and remove them after their work is merged.
  Source recovery, matching, renderer evidence and platform work feed the same
  source tree and proof ledger; retained experiments are not alternate products.
- One integrator owns canonical recovery state and commits. Parallel work must
  have explicit ownership of disjoint files/functions, private scratch space, and
  a compact provenance handoff.
- Scale independent reasoning across tens of narrow Luna workers (start with
  24 on this host), under one integrator. Increase only while disjoint useful
  tasks and integration capacity exist; machine core count is not a task quota.
  Give a shared uncertain blocker to one Sol investigator. Reassign finished
  workers while integration proceeds; do not wait for an entire wave or an
  optional trailing audit before integrating a ready batch.
- Workers use focused packets, existing source/evidence and private scratch;
  they do not rebuild the census, run full regression, edit shared registries or
  spawn further workers without the integrator's assignment. Count reasoning
  concurrency separately from simultaneous compiler jobs and canonical writes.
- With native collaboration tools, `send_message` only notifies an existing
  worker; use `followup_task` to restart a completed worker with a new bounded
  assignment. Do not wait for a queued notification to produce work.
- Handoffs name exact IDs and source ownership, base and input hashes, existing
  binding/native evidence, one focused receipt, compiler-call count, and the
  first blocker. Link large reports; don't paste ledgers or hash inventories.
- Before committing hash-pinned evidence, check that staged Git blobs preserve
  the verified file bytes; use the existing exact-byte attributes where needed.
- Default to GPT-6 Luna xhigh for bounded reconstruction and compiler work. Use
  GPT-6.1 Sol high when the explanation is uncertain or one shared question can
  unlock several regions. Prefer deterministic verification over extra model
  review; do not add a retry ladder, routing ledger, or automatic difficulty score.
- Use tools under C:\tools. Inspect existing dependencies before adding any.
  External tools need documented origin, version, and SHA-256; do not patch them.
- Begin with a falsifiable question and the smallest useful original-backed
  experiment. Keep durable findings compact. Disposable packets and receipts do
  not grant acceptance.
- Use python scripts/region.py packet <region> when indexed; add --more for
  wider context. See docs/workflow.md for the current work loop.
