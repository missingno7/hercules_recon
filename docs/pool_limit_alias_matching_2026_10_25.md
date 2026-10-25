# WORD pool limits and compiler-renaming proof, 2026-10-25

Three unchanged ENG1 resetters (`27490/279a0/27f50`, 80 bytes each) and the shared
normal initializer (`27520`, 176 bytes) gained complete relocated acceptance.
The ledger contains 421 accepted native entries / 41,232 bytes: 136 RAW rows,
242 RELOCATED rows and 43 retained linked rows. Every earlier accepted function's
source, bytes, category and binding map remain unchanged.

The three limits already had unique canonical storage in
`src/eng1/behavioral/frame_data.c`. Legacy callers use `g_normal_limit_primary`,
`g_middle_limit_primary` and `g_alternate_limit_primary`; the actual definitions
are `g_configuration_word_713b4`, `g_configuration_word_71432` and
`g_configuration_word_713ba`. The existing source graph already maps those names.
No storage is duplicated, renamed or moved to qualify for matching.

The independent native configuration writes at `281eb/2822a/281dd` identify the
three roots, outside all selected functions. Normal count comes from row WORD+2;
middle/alternate counts come from WORD+4/+6 plus five modulo 16 bits. The initial
linear scan observed 33 direct WORD accesses. Cross-checking the original HIGHLOW
fields found one more alternate-count load at `1b1d8`, decoded from the independent
function entry `1b1d0` after an inline jump table. All 34 fields directly naming
these roots are WORD accesses (15 normal, nine middle, ten alternate). The unchanged owner freshly compiles
to complete two-byte COMMON objects. Original loader-zero, directory, fixup,
TLS/load-config and native-anchor checks remain mandatory. No array capacity or
exhaustive computed-alias claim is inferred from those observations.

`scripts/storage_alias.py` supplies a separate, narrow proof for these legacy
names. A binding carries this explicit adapter description:

```json
"source_alias": {
  "kind": "compile-renaming",
  "header": "src/eng1/behavioral/actor_pool_limit_aliases.h",
  "renames": {
    "g_normal_limit_primary": "g_configuration_word_713b4",
    "g_middle_limit_primary": "g_configuration_word_71432",
    "g_alternate_limit_primary": "g_configuration_word_713ba"
  }
}
```

The verifier freshly compiles the original caller and an ignored include wrapper
that applies the pinned canonical header. It compares all nine functions in the
unit, every other executable section, all non-code storage and linker directives,
and the external symbol inventory. Only the explicitly named undefined strong
data references may differ. The comparison normalizes relocation **names**;
neither compiled object nor its instruction bytes are edited. The normal native
instruction/reference gate then checks the original contribution against the
oracle with the independently compiled actual owner. Compiler renaming supplies
no native storage identity by itself.

The header and every added include must be independently pinned. The generated
wrapper, its input closure and both compiler objects are frozen for the run and
rechecked immediately before publication. Mapping chains, duplicate destinations,
pre-existing target symbols, defined-object merging, function/weak aliases and
changed code/data/directives reject. The first implementation supports complete
WORD scalar aliases only. Same-name pointer bindings retain their existing path;
pointer views and indexed BSS are not enabled. There is no new RAW adoption path.

Twenty-seven new adversarial tests cover the bridge, ownership/width controls,
other-function and executable-storage changes, stale/missing inputs and races
during compilation or before publication. All 245 Python tests pass. All 380
strict proofs were freshly verified after the final guard additions; 43 legacy
linked function checks pass, with the existing 86 broader provenance drifts
unchanged. Sixteen wrong-reference controls reject with zero non-reference
instruction differences.

`scripts/pool_reset_differential.py` adds 624 guarded original/source cases, 208
per resetter. Cleanup/initializer services are modeled, including zero, positive
and negative return values; limits can shrink/grow and pool pointers can switch
banks during either service stage. The current actor pointer survives the first
call, the next iteration reloads the pool base, and the limit reload occurs
after the finishing call. Complete fixture banks, ordered call pointers, live
globals, cdecl frames, ESP and callee-saved values agree. The ABI returns void;
EAX is excluded. Four readable synthetic slots per bank and shrinking `65535`
cases establish no native pool capacity or full cleanup/initializer behavior.
No original host/game startup or OS module load occurs.

The additional entry checks for a free alternate slot and, when the pool is full,
releases the first matching kind. Its private C draft reaches the observed
96-byte span but retains 17 positional differences and different relocation
offsets. It remains ignored and unaccepted. This audit shows why a section-wide
linear scan must be checked against relocation fields and known function entries.

Subsequent work resolved that draft's cursor lifetime difference, added a real
provider, and accepted the complete contribution. See the
[pressure-helper follow-up](pool_pressure_matching_2026_10_25.md); the earlier
frozen census remains a record of its then-unaccepted draft.

The shared initializer row remains historical matching evidence. It does not
replace or validate a separate source-graph provider by itself. Source-graph
provider inputs and alias settings remain unchanged; no new link closure is
claimed. Header/owner changes require reviewed fresh proofs; do not edit pinned
adapters merely to add another alias family.

Evidence is in `evidence/pool_limit_alias_references_2026_10_25.json`,
`evidence/pool_limit_alias_bindings_2026_10_25.json`, the supplemental
`evidence/pool_limit_reference_census_2026_10_25.json` and the frozen final receipt
`evidence/pool_limit_alias_progress_2026_10_25.json`. Recovery.json alone grants
acceptance. Full matching remains incomplete.
