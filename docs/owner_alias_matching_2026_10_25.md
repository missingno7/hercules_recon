# Pointer owner proofs, 2026-10-25

Ten existing ENG1 caller contributions now pass the complete relocated gate:
1,120 bytes, bringing recovery.json to 438 distinct accepted entries / 43,568
bytes. The 428 prior accepted records are identical. There are 137 RAW rows,
258 RELOCATED rows / 29,360 bytes, and 43 retained linked rows. These are function
proofs, not a source-completeness or whole-image claim.

The falsifiable question was whether a legacy caller data name can be connected
to its existing actual pointer-cell owner without changing any caller code.
The earlier caller-renaming experiment changed complete function contributions
in all three tested units. Renaming just the data-only owner preserved both
tested owner inventories. The new mode explicitly proves that narrower case.

| ENG1 RVA | Complete bytes | Existing caller source under calibration/eng1/ |
| --- | ---: | --- |
| 0x36ce0 | 48 | player_unit_36030.c |
| 0x37400 | 96 | player_unit_36030.c |
| 0x37770 | 128 | player_unit_36030.c |
| 0x37f80 | 32 | player_unit_36030.c |
| 0x39460 | 208 | player_unit_36030.c |
| 0x399c0 | 48 | player_unit_36030.c |
| 0x12b00 | 256 | pickup_lift_unit_12620.c |
| 0x3c4d0 | 64 | player_scene_3a040.c |
| 0x3c510 | 176 | player_scene_3a040.c |
| 0x3c5c0 | 64 | player_scene_3a040.c |

Every caller uses the complete normal-pool pointer cell at 0x72860. The 0x3c4d0
caller also uses the collision pointer cell at 0x7217c. Their independent
allocation/configuration evidence and actual canonical owners are reused; no
storage is invented from caller operands or from an uninitialized zero value.
The caller sources, flags, gameplay headers and actual storage definitions are
unchanged. Only diagnostic co-definition metadata for the three units is
refreshed when their selected rows leave CODEGEN_SIMILAR.

The explicit binding is:

```json
"source_alias": {
  "kind": "owner-compile-renaming",
  "header": "src/eng1/behavioral/actor_pool_pointer_owner_alias.h",
  "renames": {"g_frame_owner_72860": "g_normal_actor_pool"},
  "pointer_types": {"g_frame_owner_72860": "void"}
}
```

The map describes caller to actual owner. The pinned header must contain only
the inverse macro, `#define g_normal_actor_pool g_frame_owner_72860`, apart from
comments. One pair per binding/header is supported. The second owner uses its
own header and the `byte` pointer type. It does not change the existing WORD
`compile-renaming` path and there is no automatic fallback between modes.

The unchanged caller must emit one undefined strong DATA symbol with no
pre-existing occurrence of the actual owner name. The original and adapted C
owners must each emit the exact four-byte COMMON definition and no functions
or executable storage. A generated ignored wrapper includes the pinned header,
declares the selected `void *` or `unsigned char *`, includes the actual owner,
and asserts pointer width four. It is freshly compiled with the original owner
flags plus `/WX`; incompatible or qualified declarations reject. Integer,
array and function-pointer owners cannot substitute for a pointer cell.

The verifier compares every section payload, flag and relocation, and the full
non-FILE symbol inventory including auxiliary/static records, after only the
one name normalization. FILE metadata is separately validated. Weak symbols,
weak linker directives, COMMON merging, retained/duplicate names and extra
storage reject. Both owner objects, their complete inputs, the pinned header
and the original caller remain guarded through publication. Distinct owners
within one caller have separate profiles and publication checks.

This proves the actual data-owner connection. The existing original-backed
native identity, scalar access, exact instruction/padding, complete relocation
and HIGHLOW checks still decide acceptance. No object or instruction bytes are
patched. Pointer views, indexed BSS, one-past BSS and new indirect control
transfers remain unsupported.

The reference review also establishes ten helper entry identities from their
own source/body evidence and external native call witnesses. Some helpers are
still diagnostic or have incomplete dependencies; their entry identity does
not transfer function acceptance or prove their runtime chains. Twenty-five
wrong-reference controls reject. No new original-runtime or semantic fixture
execution is claimed for this batch.

Verification passes 397 strict proofs, all 43 retained linked function checks,
and 274 Python tests, including 29 new adversarial owner-mode tests. Those tests
cover type mismatches using the real historical compiler, whole-object changes,
weak/duplicate/merged symbols, malformed metadata, input/object races, and two
distinct owner profiles in one caller. The existing 27 WORD-alias tests still
pass. The 86 previously recorded broader linked provenance drifts remain.

Frozen references and bindings are
`evidence/owner_alias_references_2026_10_25.json` and
`evidence/owner_alias_bindings_2026_10_25.json`; do not edit their accepted inputs
without protecting every dependent proof. The final census, source-graph
comparison and receipt pins are in `evidence/owner_alias_progress_2026_10_25.json`.
Scratch compilation objects and negative controls remain under ignored
`work/owner_alias_2026_10_25/` and `build/match/`.
