# Working contract

Reconstruct the 1997 Windows Hercules game with historical C/C++ tools. The PC
binaries are the matching oracle. PSX builds provide secondary evidence only.
No port, modernization, renderer replacement, or gameplay changes in this phase.

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

