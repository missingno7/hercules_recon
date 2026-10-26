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

An explicit `bss` binding can identify a reviewed loader-zero object. It requires
a separately pinned owner source/flags/symbol profile, freshly compiled on every
verification run. The owner must emit one unambiguous, exact-size COMMON data
definition under the same COFF name, or one of the explicit compiler-renaming
proofs described below. All owner includes must be pinned by that
binding's evidence manifest. The native extent must lie wholly inside one
writable, non-executable zero tail within SizeOfImage, outside loader fixups,
data directories and unreviewed TLS/load-config effects.

The binding also pins one complete independent native MOV/LEA/PUSH address
instruction and its exact HIGHLOW operand fixup. It must identify the owner base
outside the compared caller and outside every selected promotion candidate.
All-zero similarity cannot select an object. Memory uses require bounded direct
scalar accesses; indexing, implicit aliases, one-past uses and implicit indirect
call authorization remain unsupported for this kind. A scalar root alone never
authorizes a call. The separate explicit callback proof below has its own
instruction/identity requirements. Owner objects and inputs are rechecked immediately
before publication. Ordinary `data` bindings continue requiring file-backed
payloads. See `docs/loader_zero_matching_2026_10_25.md` and the published bindings.

A two-byte scalar `bss` reference may carry a `source_alias` record with
`kind: compile-renaming`, a pinned canonical header and an explicit one-to-one
map of C data identifiers. The legacy caller names must be undefined strong data,
and the actual owners must be complete WORD COMMON definitions. The verifier
freshly compiles the unchanged caller normally and through the header, then
requires every function, other executable section, non-code contribution,
linker directive and external symbol to be identical after only those data names
are normalized. No instruction or object bytes are patched. Both objects and
their complete input closures are checked through publication. The original
native identity, zero-tail, full-instruction, reference and HIGHLOW requirements
still apply. This is not name-based or zero-based alias inference. Pointer views,
weak aliases, COMMON-object merging and broader alias forms remain unsupported.
See `docs/pool_limit_alias_matching_2026_10_25.md`.

A four-byte pointer-cell alias may instead use `kind: owner-compile-renaming`.
It permits one caller/owner C-name pair and an explicit `pointer_types` choice
of `void` or `byte`. The caller must contain an undefined strong DATA symbol;
its actual owner name must not already occur in the caller. The separately
compiled C owner must emit the exact four-byte COMMON object and no functions
or executable storage. Its pinned header contains only the inverse macro map.
An ignored wrapper declares the specified pointer before including that owner,
asserts pointer width four, and compiles under the original owner profile plus
`/WX`. Integer, array, function-pointer and incompatible/qualified declarations
reject. The full owner sections, relocations and every non-FILE symbol/auxiliary
must agree after that single name rename; FILE metadata is validated separately.
Both COMMON definitions, all other storage and directives remain accounted for.
No COMMON merging, weak symbols/linker aliases or extra storage is allowed.
The original caller is compiled unchanged. Fresh original/adapted owner objects,
both input closures and the caller are guarded through publication. The same
native identity, scalar access, full-instruction, relocation and HIGHLOW gates
then apply. This does not enable pointer views, indexed BSS, one-past BSS or new
indirect control transfers. See `docs/owner_alias_matching_2026_10_25.md`.

An explicit `owner_storage: zero-section-v1` instead proves a real initialized-zero
data-only C owner such as `g_engine_interface = {0}`. The unchanged source must
emit exactly one strong external object at offset zero of one complete `.bss`
section of the reviewed size. The section has no raw payload, relocations,
COMDAT/alias metadata or other storage; only known read/write, uninitialized-data
and alignment flags are allowed. Other objects, functions, unsafe directives,
unknown profiles and aliases reject. Native zero-tail, complete independent
HIGHLOW anchor, source/include freshness and bounded direct scalar accesses
remain mandatory. The default COMMON path is unchanged. See the
[interface transfer review](../evidence/interface_transfer_review_2026_10_25.json)
and [measured continuation](matching_realignment_2026_10_25.md).

Current support is intentionally conservative: i386 REL32 direct entry calls/jumps
and DIR32 function/data/IAT/local-table references in complete /Gy contributions.
Other BSS forms, relocated data tables, ambiguous aliases, weak symbols, register
indirect dispatch and other unsupported forms remain unaccepted. Do not remove
these checks to increase a count. Complete compiler padding is checked; padding
inserted outside the contribution by the linker is outside this proof.

An optional `indirect_calls` inventory can separately prove the narrowly defined
`rooted-callback-v1` expression. Each entry names one CALL and one earlier root
load, a complete independently verified four-byte scalar `bss` root, low9-bit
index mask, stride8 or12, and an independent native witness window/hash. The
caller and original must both contain the exact straight-line root load, mask,
one DWORD register argument, optional x3 LEA, slot-zero memory CALL, optional
count reload and four-byte caller cleanup. Root/index/argument registers are
disjoint; no instruction can clobber their provenance. Direct or local-table
entries inside that window reject. An independent file-backed native window
must corroborate the same root/mask/stride/call/cleanup shape with exactly one
HIGHLOW root field; it cannot overlap this caller or any selected promotion
candidate. All other instructions, references, owner proofs and complete bytes
still pass their original gates. Unreviewed calls and register-indirect calls
remain unsupported. This proves the dispatch expression, not dynamic callee
identities, callback behavior or pointee capacity. See
[the appender and mode review](action_appender_matching_2026_10_25.md).

The opt-in `profile: fixed-slot-v1` inventory covers only complete argument-
forwarding FF15 CALL/FF25 tail-JMP wrappers through an accepted 1000-byte
loader-zero interface root (exact-size COMMON or `zero-section-v1`). It requires exact DIR32 root/slot provenance,
native and compiled argument order, cleanup and RET, and an agreeing module/slot
in the hash-pinned [static slot review](../evidence/interface_slot_review_2026_10_26.json).
`python scripts/interface_slots.py check` reproduces the loader copy, mutation
audit and provider stack measurements; `bindings` writes opt-in candidates under
`work/iface_bindings/`. Conflict and unresolved providers remain rejected. This
proves physical stack compatibility of the statically copied table, not C
prototypes, types, semantics or dynamic callback behavior; the address audit
cannot exclude arbitrary computed writes. All existing owner, instruction,
reference and complete-byte checks still apply. Each module's accepted root
record is reused verbatim.

The proposed `same-unit-bss-v1` COFF-only form remains unsupported. VC5 data
symbols in the tested `.bss` units have offsets but no per-object size records.
The next symbol bounds an allocation interval including padding: in fresh
`u0cc90.c`, the four-byte `g_2a3ac` starts at offset 1504, and the next symbol
starts at 1512. Assigning eight bytes to that scalar is unsound. A native base
anchor does not close that capacity gap. Menu statics also occupy file-backed
native `.data`, before the zero tail. Exact same-invocation allocation evidence,
independent native endpoints and object placement are still required; neither
section offsets nor a caller-implied address supplies them.

An explicit data symbol record with `data_profile: relocated-data-v1` may use
`payload_bindings: {symbols: {...}}` for its own initialized DIR32 fields.
Each nested symbol carries independent evidence, kind/RVA/size, and its exact
`references` inventory; offsets are relative to the containing payload, not
the section. Data (including strings) has a complete native SHA-256. The new
module compares every initialized allocation byte through the next object or
section boundary, including padding, and rejects ambiguous starts and truncated
allocations. Every relocation must occupy one complete disjoint four-byte field,
match the reviewed symbol/type/addend, and contain `image_base + bound RVA +
addend` in the native image. When present, the native HIGHLOW inventory must also
agree exactly. Function, data and IAT identities retain their ordinary checks;
defined nested payloads recurse, with shared symbol identities and section bases.
Cyclic graphs, BSS, section symbols, weak aliases, interior function/import
addends, one-past pointers and non-DIR32 data fixups remain unsupported. No PE
base-relocation directory is required for the stripped EXE. This verifies the
payload and its references, not source identity. The current ENG3 `_g_26110`
18-record source allocation cannot represent the reviewed 10-record native
table; relocation support does not authorize comparing a 200-byte prefix.

The function binding record may opt in to
`coff_identity: {profile: msvc-undef-comdat-v1, symbol: NAME}` only for the
compared scalar-deleting destructor (`??_G...`). Exactly two same-name records
must exist: plain external function UNDEF at value zero and one plain external
function definition at zero in a complete single-function `.text` COMDAT.
Its section auxiliary must describe non-associative `ANY` selection, with exact
size and relocation count. No additional definitions, weak/alias records or
other duplicate names gain support. Both indices denote the same linker name,
so either relocation index resolves to its one independently bound identity;
all original complete-code, operand, base and relocation checks still apply.
A fresh VC5 `/O2 /Gy` probe reproduces the pair for an out-of-line virtual
destructor whose vftable refers forward to its generated deleting destructor;
the unused inline-virtual and nonvirtual controls do not emit that pair. This
observed trigger is not a claim about all VC5 optimization/link configurations.
Eight original destructor comparisons pass with prior independent callee review;
their diagnostic receipts do not promote those still-unaccepted callee records.

Loader-zero records can opt in to `anchor_profile: cmp-test-absolute-v1`.
The separate `zero_compare_anchor.py` preserves the pinned `zero_storage.py`
storage, native hash, independent span, decode and exact HIGHLOW checks. Only a
complete CMP/TEST instruction's absolute, unindexed, unsegmented memory operand
is a new address witness; its width must fit the independently compiled owner.
Immediate constants and base/index-relative operands cannot identify storage.
The native ENG3 mode-cell instruction at RVA `0x5d89` is eight bytes, including
its final immediate; the prior seven-byte receipt is rejected. Native anchor
verification does not close any caller's other unbound references.

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
