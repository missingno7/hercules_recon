# Matching strategy (living record)

Integrator-owned, updated when a discovery changes priorities. recovery.json is the only
acceptance authority; this file records why work is ordered as it is. Measured state and
receipts live in evidence/ and in the dated docs; do not pin this file from evidence.

## Frontier (2026-10-31, census on 9ff6ae7: work/blockers/queue_l.json)

Accepted 1,140. Pending masked rows (CODEGEN_SIMILAR) 955; RECONSTRUCTED 149. Of 943 census rows,
11 had no identity blocker and no indirect transfer; most rows are blocked by several kinds.

Blocker families, by rows touched (a row counts once per family):

| Family | Rows touched | Rows blocked only by it | Lever |
| --- | --- | --- | --- |
| Callee recorded under another name (alias) | 223 | 12 | canonical naming migration, caller renames (REL32 refresh) |
| Callee with no ledger row at that address | 345 | 39 | reconstruct the callee in its owning unit (new rows) |
| Callee row exists but is not masked-equal | 114 | 9 | near-callee matching (often VC5 context effects) |
| Zero storage without an owner/anchor (data_zero) | 223 single-kind rows | - | loader-zero forms; owners + anchors |
| Initialized data (data_file) | 50 single-kind rows, ~11 KB | 50 | data-owner form with typed initializers |
| Register/register-based indirect CALL | 153 rows contain one | 35 | opaque-indirect-call-v1 (2026-10-31) |
| EXE statically linked library callees | ~60 rows | 19+ | closed library-member identity (not yet proven) |

Accepted after the first wave-j integration: **1,179**.

Highest single gates: ENG1 0x26de0 (16 callers), shared remove_actor_preserving_resource
(ENG1 0x27410 + ENG3 0x1cd40, 18), TITLE title_054f0 (20) and title_164b0 (14), EXE operator delete
(14) and CDialog::EndDialog (10).

## Established compiler insights

- VC5 C2 orders commutative address operands (SIB base/index) by a descending key that compares
  folded symbol identity fold16(n)=((n&0xffff)^(n>>16))&0xffff with the temporary id; preceding
  declarations raise symbol identity. A real header that the unit plausibly included (for example
  <stdlib.h>) can therefore fix operand order without touching the function body
  (work/velocity_i/solSIB/findings.md; applied to actor_kind_create.c and actor_middle_create.c).
- Register, SIB and commutative tie-breaks depend on the unit's declaration count
  (work/velocity_e/sched/ROUND3.md); trace before editing (work/velocity_e/vc5model_sol/).
- Per-read volatile steering is owner-approved only per access with native evidence (0x171b0 held_5c);
  wave j extends it to two more held_5c reads in mode_handlers_16840.cpp with the same evidence class.
- VC5 C2 sorts every commutative input list (address sums, OR/AND chains of comparisons) in descending order
  of a 32-bit node key `(P << 24) | (W << 16) | L`:
  * P is an order-dependent priority built from the children.
  * W is the 8-bit sum of the children's W values.
  * L is the XOR of the children's low halves with the node's opcode; leaves carry symbol identity or a
    temporary id.
  Reordering the source terms alone does not change the output. Changing tree shape, operand sides or real
  unit context does (work/velocity_j/solCMP/findings.md: eng3_1f0a0 masked-equal, eng3_1f130 raw-equal).
  Predict with this key before compiling.
- Indexed pre-increment loops (`p[++i]` rather than a pointer walk) restore VC5's native loop peeling
  and induction-update order (solN1: _eng1_26de0, _eng1_26cd0).
- A store/reload chain to a local outranks independent stack cleanup in VC5 scheduling (solTitle:
  title_164b0's last 17 bytes); no source form reproducing the native order has been found yet.
- The solVC5 classifier (work/velocity_j/solVC5/classify.py) puts the 150 non-equal rows into families:
  3 pure SIB swaps, 10 register-only, 134 mixed. A header that precedes the declarations changes SIB roles;
  total declaration count does not.

## Proof forms added this campaign

- loader-zero-indexed-v1 / loader-zero-paired-v1: address identity of loader-zero owners with a
  proven minimum extent (no capacity claim).
- opaque-indirect-call-v1 (scripts/opaque_call.py): admits FF /2 near CALLs through a register,
  register-based memory, or a proven data/BSS cell. Proves instruction identity only; every record
  states target "unidentified" and abi "unclaimed". JMP, prefixed, far and relocated register-based
  forms stay refused. Its cell check caught two masked-equal wrappers (eng3.shared_1ffa0,
  eng3.shared_20120) whose source calls interface slot 0x20 where the native calls 0x2ac.
- compile-renaming-byte-v1 (scripts/storage_alias_byte.py): one-byte scalar aliases with a /WX
  caller-type check, otherwise the unchanged word alias proof.

## Identity evidence: the binding constraint (wave j)

Independent anchors, not owners or content, now limit storage identities:
- Zero storage (solZ): 523 DLL symbols. 61 ready, 28 need only an owner, 230 have no anchor outside
  pending rows, 204 involve aliases or sub-objects.
- File data (dataform, prototype form `file-data-owner-v1`): 45 of 48 metadata rows lack a start anchor.
  One pilot (eng3.24f50) verifies end to end.
- The pinned design (docs/loader_zero_matching_2026_10_25.md) requires anchors outside the compared and
  selected callers. The worker contract also keeps them outside all pending rows, because the per-run
  overlap check would otherwise block the anchor-containing row's own later promotion.
- A non-weakening alternative (re-anchoring rules or a compiled-layout proof) is under design
  (work/velocity_j/anchors2).
- HERCULES.EXE is relocation-stripped, so no HIGHLOW fixups exist. EXE storage needs a different form, and
  the EndDialog library closure (work/velocity_j/solEXE) waits on the 0x215c90 restart cell.

## Failed or exhausted approaches (do not repeat without new evidence)

- host_file_open_only_3c50: historical missing-return codegen recorded
  (evidence/host_file_open_3c50_history_2026_10_31.json); canonical source keeps `return opened;`.
- eng3.eng3_1b4a0: local lifetime/frame shape; no context hypothesis closed it (wave i n01).
- Near rows far from equality (hundreds of differing bytes): eng3_24bb0, eng3_24870, eng3_1b650,
  eng1_32810, title_054f0, eng1 sprite_unit_03cd0 rows (wave j). Do not grind them without a mechanism.
- New-function attempts that stayed far: 0x19b40 (272 vs 224 bytes), 0x2b130 (425 bytes), 0x41000/0x41da0
  (wave j Luna; Sol retries in solN2).
- EXE identities withdrawn on 2026-10-27 stay withdrawn until a closed library-member proof exists;
  _cleanup_native_4b50 must not be reused.

## Workstreams (next)

1. Done: opaque-call and byte-alias acceptances (1,179).
2. Re-pin batch b11 for the wave-j units, adding new rows for the reconstructed callees and retiring the
   behavioural duplicates in config/eng1/sources.json; then bind their callers.
3. Canonical naming migration (aliases, twin wrappers, duplicate providers in config/eng1/sources.json).
4. Data-owner form for initialized metadata tables (typed initializers, no asset payloads).
5. VC5 mechanism families from the residual-difference classifier; apply only validated mechanisms.
6. EXE library closure research (MFC/CRT members) under the 2026-10-27 audit constraints.
