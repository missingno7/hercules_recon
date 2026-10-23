# Matching and proof limits

recovery.json is the authoritative state for accepted function matches. Existing
FUNCTION_MATCH entries retain the strict raw-byte definition. Keep generated
objects and reports under ignored work/ or candidates/ paths.

## Matching outcomes

| Outcome | What it establishes |
| --- | --- |
| RAW, stored under the legacy FUNCTION_MATCH name | Fresh pinned-toolchain compilation produced the complete original function bytes exactly, under the existing raw acceptance gate. |
| FUNCTION_MATCH_RELOCATED | Fresh complete instruction match where every COFF relocation is independently bound and verified; original function/data addresses may differ in a new link. |
| CODEGEN_SIMILAR / NEAR | Code bytes differ, or one or more references remain unresolved or ambiguous. Diagnostic only. |

FUNCTION_MATCH is the legacy ledger name for RAW. Keep its acceptance and
regression behavior unchanged. A masked equality,
same-size function, relocation-derived address, successful link, or constrained
layout result is not enough to accept a relocated function.

A relocated acceptance must compare the complete compiler-emitted function
extent. Only genuine linker-resolved operand fields may differ. Instruction
lengths, opcodes, encodings, branches, constants, and all other compiler-emitted
bytes must match. Independently establish each referenced function, data object,
import, thunk, table, alias, addend, and interior offset where applicable.
Reject missing, ambiguous, conflicting, and unsupported reference forms.

The verifier works from real translation-unit context, preserves
internal branch and jump-table semantics, detects wrong symbol identities, and
protects all prior raw matches. The relocation operand alone cannot prove which
symbol it names. Do not promote based only on the masked.py diagnostic or the
narrow constrained TITLE call pilot.

Use `promote.py --relocated --bindings evidence/relocated_reference_bindings.json`
with reviewed existing IDs. Add `--verify-only` for a dry run. `match.py verify`
checks RAW and RELOCATED together; `--raw-only` independently checks RAW,
including retained raw proofs on two legacy LINKED_MATCH rows. `linked.py verify`
retains all 43 older linked claims without making layout a new milestone.

Bindings identify every use by COFF name, relocation type, operand offset and
addend. Data pins the complete declared payload and bounded interior offsets;
imports pin PE IAT DLL/name or ordinal identity. Reviewed immediate end pointers
may explicitly use a data object's one-past bound. In-function jump tables require
complete reviewed extents and destinations at decoded instruction boundaries.
Each acceptance pins source/include context, compiled bytes, target, reference
bindings and independent evidence. Promotion recompiles every accepted peer and
uses the existing recovery transaction.

Current support is intentionally conservative: i386 REL32 direct entry calls/jumps
and DIR32 function/data/IAT/local-table references in complete /Gy contributions.
BSS payloads, relocated data tables, ambiguous aliases, weak symbols, register
indirect dispatch and other unsupported forms remain unaccepted. Do not remove
these checks to increase a count. Complete compiler padding is checked; padding
inserted outside the contribution by the linker is outside this proof.

## Keep evidence dimensions separate

Semantic evidence is recorded separately: SEMANTIC_VERIFIED means behavior has
independent support; it does not grant RAW or relocated acceptance. Source
coverage is separate too: SOURCE_RECOVERED and INCOMPLETE distinguish present
source from missing or provisional implementations. Dependency closure and
runtime completeness are also separate. Exact code bytes do not prove whole-game
behavior or source identity. A semantic correction is valuable before it matches.
A successful source link proves only that its selected inputs have no unresolved
dependencies.

Counts in dated reports describe their recorded checkout and inputs. Read current
accepted state from recovery.json.
