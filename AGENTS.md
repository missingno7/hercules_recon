# Working contract

Reconstruct the 1997 Windows Hercules game with historical C/C++ tools. The PC
binaries are the matching oracle. PSX builds provide secondary evidence only.
Historical reconstruction and a modern drop-in renderer now develop together
inside this repository. Original simulation and gameplay remain unchanged.

Current user priority is reverse engineering and reconstructing the original
game first. Recover coherent source and dependencies in forms useful to later
matching decompilation. Further renderer implementation is deferred; it should
follow from the recovered game rather than drive approximations or isolated
preview milestones. A preview is not a runnable-game completion criterion.

## Ownership and proof boundaries

- `src/`, `calibration/`, and `recovery.json` retain the historical reconstruction
  rules below. Modern implementation never grants historical acceptance.
- `renderer_contract/` owns evidence-backed, GPU-independent original interfaces
  and reference utilities. Correct unknown semantics here from original evidence
  before consuming them in a renderer; never silently redefine them downstream.
- `dropin_renderer/` owns modern C++ build, forwarding, diagnostic capture and
  backend code. Consume the shared contract directly; do not copy its package.
  Implement bounded vertical slices with explicit unsupported paths. Keep original
  screen-space behavior separate from private native-resolution policies.
- Forwarding to original callbacks/APIs is permitted only in the modern reference
  and diagnostic integration layer; it is never historical source recovery.
  Do not patch original code, embed machine code, or change original game files.
- Preserve synchronous callback ordering and live-link/state reloads. A borrowed
  frame entry snapshot is not a consumption trace or an asynchronous owned scene.
- Original post-entry execution still needs explicit execution authorization,
  verified writable-path/profile/registry isolation, windowed startup and display
  restoration. A synthetic fixture or prior pre-entry probe does not satisfy that
  gate. Continue independent offline and source-built work while it is unresolved.
- GPU screenshots, synthetic forwarding and finite semantic comparisons establish
  only their measured boundaries, not original-host or whole-renderer equivalence.

## Historical reconstruction

- Never modify `assets/`. Never commit originals, extracted game files, tools,
  generated builds, databases, or large disassemblies.
- Follow evidence -> scratch experiment -> strict acceptance. Keep experiments in
  ignored `candidates/` or `work/`; do not copy entire source trees per experiment.
- `recovery.json` is the sole authoritative function recovery state. Evidence,
  toolchain provenance, commands, and confidence belong in compact durable files.
- Fresh historical compilation and actual target-byte equality are required for
  `FUNCTION_MATCH`. Relocation-normalized similarity is diagnostic, never exact.
- No copied machine code, byte-array functions, patching, original-binary dispatch,
  arbitrary assembly/placement tricks, or unsupported compiler-steering hacks.
- Canonical source changes require measured evidence and regression verification.
  Previously accepted matches must remain exact. Treat compiler/library code as
  library code and identify it instead of casually rewriting it.
- Keep claims narrower than proof: function equality does not prove module layout,
  linker flags, source identity, or whole-binary equality.
- Agents own disjoint files/functions, use private scratch directories, and report
  provenance. One integrator changes canonical state and makes Git commits.
- Use tools under `C:\tools`; inspect existing tools before adding dependencies.
  External tools need documented origin, version, and SHA-256. Do not patch them.

## Workers and context

- Default to GPT-6 Luna **xhigh** for bounded reconstruction and compiler iteration.
  Use GPT-6.1 Sol **high** when the explanation is uncertain or a shared question
  has leverage; hand diagnosed experiments back to Luna. Other tiers are exceptional.
  Neither size, similarity nor a first failed candidate determines routing.
- Do not add model review after deterministic acceptance, a retry ladder, routing
  ledger, or automatic difficulty scoring. Every model passes the same gate.
- Choose coherent regions while shared representation is uncertain, then shrink
  to local hypotheses. Do not rewrite relocation-only equals to solve link layout;
  recover the real missing contributions and dependencies.
- Start with `python scripts/region.py packet <region>` when indexed; use `--more`
  for spans. See `docs/workflow.md` for private receipts and compact handoffs.
  Packets are disposable context; `recovery.json` alone decides acceptance.
- Record a prediction and falsifier before a family. Reopen exhausted work only
  with new evidence, a new discriminator, or changed context. Keep outcome identity
  separate from source spelling and accepted proof. Preserve exact peers.

