# Reconstruction goal

The project is a complete, faithful native source port of Disney's 1997 Windows
Hercules game, eventually using SDL3 and modern compilers. The original PC
binaries remain the evidence oracle. The native game must not rely on original
first-party executable modules.

Historical matching is a foundation: reconstruct source that the pinned
historical compiler translates into the original function's exact instructions.
The linker may choose different addresses. Original executable layout, whole
image equality, and original RVAs are not acceptance requirements.

## Independent proof dimensions

- FUNCTION_MATCH is the legacy ledger name for RAW, the established strict raw
  category. Preserve all accepted claims and their existing fresh-build/raw-byte
  gate.
- FUNCTION_MATCH_RELOCATED is the accepted category for exact instructions whose
  linker-resolved references differ. It requires independent proof of every
  symbol identity, relocation type, addend, and interior offset. The verifier and
  journaled promotion path are integrated into match.py and promote.py. Masked
  comparisons and constrained-link pilots alone do not grant this status.
- CODEGEN_SIMILAR or NEAR means instructions differ or references are unresolved.
  It is not a completed match.
- SEMANTIC_VERIFIED describes independently established behavior. It does not
  imply instruction matching.
- SOURCE_RECOVERED and INCOMPLETE distinguish source coverage from missing or
  provisional implementations.

These dimensions must not be collapsed. A function may have source without an
accepted match. A matched function may still have incomplete callers, data
providers, link closure, or runtime behavior. A normal source link proves
dependency closure only for its inputs.

## Reconstruction priorities

Recover actual source, shared state, data ownership, ABI details, and coherent
dependency families. Preserve the behavior exposed by original code and finite
differential evidence. Use source-built modules and bounded tests to close
dependencies and catch mistakes.

Advance exact function matching when the compiler output and reference evidence
support it. Do not spend effort reproducing link placement for its own sake, and
do not let a single code-generation mismatch prevent useful source recovery
elsewhere.

The progression is:

    original PC evidence
    independently verified matching source
    complete first-party source-built game
    replaceable platform boundaries
    SDL3 native runtime

Keep platform work outside recovered gameplay logic. Introduce small boundaries
when actual call paths require them. Preserve original render commands and their
ordering while allowing a modern backend. Do not build broad architecture before
the recovered code needs it.

## Integrity rules

Never change original assets or game binaries. Never use copied instructions,
patched machine code, original-binary dispatch, guessed gameplay, or success
stubs to claim source coverage or matching. Keep incomplete diagnostic
placeholders out of production builds. Do not start autonomous gameplay-playing
agents; prefer static reconstruction, existing bounded differentials, and
human-recorded end-to-end regressions.

See [matching](matching.md), [workflow](workflow.md), and the repository
[working contract](../AGENTS.md) for operational rules.
