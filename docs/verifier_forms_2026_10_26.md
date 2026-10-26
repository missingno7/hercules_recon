# Opt-in verifier forms added 26 October 2026 (velocity_c)

These profiles extend [matching proof rules](matching.md) without changing it:
accepted proofs pin that file byte-for-byte. Implementations: `scripts/relocated_data.py`,
`scripts/coff_identity.py`, `scripts/zero_compare_anchor.py`; tests in
`tests/test_verifier_forms.py`. Each is opt-in through an explicit binding field.

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

