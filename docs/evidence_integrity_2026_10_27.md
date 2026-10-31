# Evidence integrity corrections (2026-10-27)

Owner ruling: STRICT must mean genuinely proven STRICT. Withdrawn claims are
listed separately from regressions; none of the changes below is a code or proof
regression.

## Withdrawn acceptances: 14 HERCULES.EXE rows

An individual audit (`evidence/exe_identity_audit_2026_10_27.json`, Sol, reviewed by the
integrator) found **0 KEEP, 14 DEMOTE**. Every row is still instruction-equal. What each row
lacks is an independently established callee or storage identity that its
FUNCTION_MATCH_RELOCATED proof claimed:

- **Operator delete at 0x11b00:** relied on by 9 scalar-deleting destructors. Its own `_free`
  call is unclosed: `__crtheap` and the lock/SBH storage are missing.
- **CWinApp and CDialog destructors:** no closed member chain reaches them.
- **CGamePlayPage and COptionsSheet destructors:** exception-handler labels and vtables are
  unbound. The earlier twin finding for these two was a census over-masking; their own shapes
  are unique.
- **`$E121`:** `theApp` zero storage has no exact-size owner or anchor.
- **Host callback cells:** used by `host_01d00`, `host_01d30`, `host_0ac40` and
  `host_frame_notify_zero_ac00`; the cells and their aggregate have no identity.

A link-order argument (re-implemented from mafia-re's crtannotate idea) places operator
delete in the library region, away from its application-code twins. That is corroboration
only: the physical member chain breaks at `_strncmp` and `_free`. No other accepted binding
uses the 14 rows.

`scripts/demote.py` performed the withdrawal as one journaled transaction. Each row goes back
to its own prior masked proof (CODEGEN_SIMILAR), and the masked co-defined inventories of the 7
affected units are recomputed. Before the write it required:
- a fresh, exact verification of every remaining RAW/RELOCATED row (995);
- the LINKED check (43 rows, 0 failures);
- masked equality of the affected units.

Each row keeps `withdrawn_acceptance`: the old proof, its binding path and hash, the review and
its hash, the missing link, and the restore command
(`promote.py --relocated <id> --bindings <path>`). The review is
`evidence/exe_identity_withdrawal_review_2026_10_27.json`.

Accepted functions: 1,050 before, **1,036** after (146 RAW, 847 RELOCATED, 43 LINKED).

## Clean-checkout reproducibility

`scripts/checkout_pins.py` (tested by `tests/test_checkout_pins.py`) clones the commit with
`--shared` under `core.autocrlf=true` and under `false`. In those checked-out bytes it checks
every accepted proof's source hash, every binding `evidence_files` pin (through
`reference_snapshots` redirects) and every `evidence/repin_snapshots` address. Uncommitted
originals (`assets/`, `work/discs`) are left to `match.py --original-root`. At `08ce1f8` it found
57,956 pins and 0 mismatches under both settings.

A full fresh verification inside a clean clone exposed two path dependencies, now fixed:

1. `scripts/fixed_slot.py` read the reviewed non-target oracles from the checkout. It now reads
   them from the root that holds the verified target. Every input is still hash-checked
   against the pinned review.
2. Eleven existing-unit RAW proofs pinned `unit_inventory_sha256`, which hashes the absolute
   checkout path in COFF FILE records. `scripts/portable_inventory.py` freshly compiled each
   unit in the recording checkout and required the original hash to reproduce exactly. Only
   then did it add `unit_inventory_portable_sha256`, the same complete inventory with FILE paths
   repository-relative. `match.py` checks the portable hash when it is present. The original
   hash stays in each proof.

Two follow-ups, both found only by verifying a fresh clone at a different path:
- The portable hash must list symbols in table order. A longer FILE path adds auxiliary
  records, which shifts every later symbol index.
- Two legacy LINKED rows, title.01dd0 and title.016e0, are verified through retained
  `prior_raw_proof` records that also pinned the absolute inventory. Both now carry the
  portable hash, recorded after the original reproduced.

Result at `6fcd741`, in a fresh `autocrlf=true` clone outside the developer checkout:
- pins: 57,533 checked, 0 problems, under both EOL settings;
- `match.py verify --original-root`: 995 of 995 RAW/RELOCATED rows exact, 183 compiles;
- LINKED: 43 rows verified, 0 failures.

## Source-fix batches through the reviewed re-pin

The re-pin landed two batches of scheduling-lane source fixes. Lanes were guided by
`scripts/funcdiff.py` and the VC5 allocator/scheduler model (work/velocity_e/vc5model_sol).
Before each re-pin transaction, a cheap pre-check compiled every candidate once and confirmed
two things: every accepted peer was byte- and relocation-identical, and no masked row regressed.

- Batch 1 (`fdea1e8`): 12 sources; 24 rows became masked-equal. Three of them were then bound
  and accepted (`0826a2a`): host_music_signal_1090, options_keyboard_reset_9330 and
  options_keyboard_toggle_92c0.
- Batch 2 (`05c0fe6`): 2 sources; 3 rows became masked-equal.
- The second re-pin exposed two fail-closed gaps in `repin.py`, now fixed with tests:
  - it overwrote earlier `reference_snapshots` redirects;
  - pins of an older, already preserved source version did not resolve to their own snapshot.
- Deferred: `handlers_10370.c` reaches masked equality only with a lean `<windows.h>` context
  hypothesis. That changes the include closure its accepted peers pin, so it needs a reviewed
  context re-proof.

Accepted functions: **1,039** (146 RAW, 850 RELOCATED, 43 LINKED). Masked rows: 1,034 verified,
0 failures. RECONSTRUCTED rows: 185 down to 170.

Compiler use for this work (2026-10-27), about 3,860 invocations in total:

| Activity | Compiles |
|---|---:|
| Lanes and investigators | 1,097 |
| Batch 1 (pre-check 24, baseline 211, re-pin 183) | 430 |
| Batch 2 (pre-check, baseline 184, re-pin 184) | 378 |
| Withdrawal transaction | 233 |
| Gate runs (full verification and test suite) | about 1,450 |
| Promotion | 223 |
| Portable-inventory migration | 40 |

## Wave f (same day)

- `scripts/fixed_disp32.py` implements the reviewed `fixed-i386-disp32-v1` profile for the
  stripped EXE. The anchor is an exact mod00/rm101 disp32 dereference inside an independently
  reviewed native function. That function must lie outside every pending row of the current
  ledger. The first binding pinned core verifier code (`scripts/match.py` among it) and a
  pending-interval snapshot that the next re-pin batch invalidated. Both acceptances were
  withdrawn with `demote.py`. The profile now checks independence against the live ledger, and
  the rows were re-bound without verifier pins. The worker contract and the integration step now
  forbid core verifier modules as binding evidence.
- Batch 3 (`7a96970`) landed nine sources; eight EXE rows became masked-equal. `eng1_34910`'s
  caller now names the accepted callee instead of a second name for it. A stale historical pin
  was satisfied by restoring its exact bytes from git history.
- A stale-CRLF evidence pin reached origin (`0e3fc8c`) because a push was gated on a pipe into
  `tail`. It was fixed forward (`b2420e8`) by redirecting the pin to its preserved snapshot.
  Both stale working copies now equal their committed bytes, and pushes are gated on the check's
  own exit code.
- Deferred for a reviewed include-context re-proof: `handlers_10370.c` and `handlers_25040.c`.
  Each matches only with a `<windows.h>` TU-context line.

Accepted functions: **1,045** (146 RAW, 856 RELOCATED, 43 LINKED). The new acceptances are
eng1_34210, eng1_34910, exe.host_02dd0, exe.host_02ea0, host_file_cancel_3ea0 and
host_music_apply_1000. RECONSTRUCTED rows are down to 162.

## Waves g and h (2026-10-28 to 2026-10-31)

Proof-form and re-pin extensions. Each one is opt-in, and default verification is unchanged:

- **Include-context re-proof** (`93313fc`). A `repin.py` plan can now declare
  `context_additions`: the exact pinned-toolchain headers that a source fix adds to the include
  closure. The full after-verification recomputes the real closure, so an undeclared header fails
  closed. `handlers_10370.c` and `handlers_25040.c` landed with a `<windows.h>` TU-context line.
  It is recorded as a measured hypothesis, not as original text.
- **Reviewed unit-inventory refresh** (`d1c5dfb`; `scripts/unit_refresh.py`,
  `reviewed-unit-delta-v1`). Two conditions apply:
  - every accepted contribution stays byte-, flag- and ordered-relocation identical;
  - the inventory delta is exactly the declared set.

  Old inventory hashes are kept in `unit_inventory_history`. It landed the interface-field fix
  in `handlers_121c0.c`, `handlers_0e020.cpp` and `handlers_28ae0.c`.
- **EXE anchor forms** (`c02022f`; `scripts/fixed_anchor_forms.py`):
  - `fixed-i386-moffs32-v1`;
  - `fixed-i386-regimm-v1`.

  Both sit beside `fixed-i386-disp32-v1`, with the same independence gate against the live
  ledger. Thirty-nine controls run on original bytes reject the interior, one-past, too-wide,
  pending-anchor, control-transfer and register-overwrite cases.
- **Evidence-producing tools that are not wired into the verifier:**
  - `scripts/data_contribution.py` (`data-contribution-v1`);
  - `scripts/local_byte_tables.py` (`coff-local-byte-table-v1`, for compiler-owned indexed
    byte tables).

Source corrections that change no generated code:

- **ENG3 `+0x14` interface field** (`19af7e1`). RVA 0x407b4 was declared as
  `g_eng3_table_407b4`. It is offset 0x14 inside the accepted `_g_engine_interface`, so it is an
  interface field, not a separate cell.
- **Shared callee name** (`b7af9e6`). Five ENG3 callers called the accepted `eng1_2a660` through
  a second name. They now use its own name.

Every one of these source changes landed through reviewed re-pins. In each, fresh before/after
verification of every accepted row was identical, and the masked rows were restaged.

Acceptances: 1,045 to **1,083**:

| Change | Count | Rows |
|---|---:|---|
| disp32 rows | 1 | `exe.host_02b50` |
| Freed by the interface-field fix | 4 | — |
| Freed by the shared-callee rename | 3 | — |
| Sonnet binding lane | 2 | — |
| Bound file-backed data payloads | 3 | — |
| GTE rows anchored in `exe.host_0b320` | 23 | — |
| `zero-section-v1` owner-form rows | 2 | — |

Before acceptance, a clean clone at a different path (`core.autocrlf=true`) verified all strict
rows and LINKED rows with 0 failures.

Recorded stops:

- **ENG1/ENG3 file-data symbols.** They have no defining source. Defining them as C initializers
  needs an owner decision.
- **Unanchored EXE zero cells:** `_g_host_callback_prefix`, `theApp`, `_g_host_pool` and the
  music bank headers.
- **TITLE same-unit `.bss` statics.**
- **`_g_kind_info` capacity.**
- **Library closure at `_free`.** It stops at the lock and SBH storage. The candidate profile is
  kept unlanded until a row can use it.
- **`eng3.142b0`.** It needs the local byte-table form wired into `match.py`; that work is in
  progress.

## Waves h and i (2026-10-31)

Verifier forms, all opt-in and dispatched from the `fixed_disp32.py` copy of the pinned comparator
(`masked.py` is unchanged; the comparator-preservation test pins every hook line):

- **`coff-local-byte-table-v1`** is wired into `match.verify_rows` (`eng3.142b0`,
  `eng1.native_code_target_012d10`).
- **`fixed-i386-indexed-v1`** (`scripts/fixed_indexed.py`): indexed `[reg+disp32]`/SIB reads of an
  independently anchored EXE loader-zero owner (`exe.host_04b10`, `exe.cleanup_native_4cc0`).
- **`loader-zero-indexed-v1`** (`scripts/loader_zero_indexed.py`, owner rulings D1+D2): the same
  for DLL loader-zero owners. The record keeps the unchanged `zero_storage` owner and native anchor
  checks and must carry `loader_zero_indexed.extent_claim: "minimum"`. That claim states that the
  exact compiled owner extent is a **proven lower bound only**. No capacity, upper bound or runtime
  index range is claimed, and none is inferred from neighbouring symbols or linker layout. An
  indexed use proves only that its relocated disp32 is `owner + addend` with `0 <= addend <
  minimum extent`. The compiled and native instructions must decode to the same pinned form, and
  the native displacement must equal the owner address plus the addend. Direct and immediate uses
  keep the legacy loader-zero checks. Tests: `tests/test_loader_zero_indexed.py`, which covers wrong
  anchors, offsets, owner extents and native instruction mismatches, with native controls on
  ENG1.DLL.

`_g_kind_info` (ENG1 0x6fee8; ENG3 0x3fbd0) is bound with an 8-byte minimum extent (element 0).
The canonical `g_kind_info[1]` declaration is **not** a proven allocation size. A native runtime
must size this table from the loader's file-driven count, not from that declaration.

Reviewed re-pins:

- **Batch 7:** `eng1_10900`, `eng3_042a0`.
- **Batch 8:** `archive_open_5130`, `host_setup_slots_reset_a1a0`, `host_engine_select`.
- **Batch 9:** `eng1_171b0`. Owner-approved per-access steering: `held_5c` is read once through a
  non-volatile lvalue, matching the original's single load and folded memory test at 0x17202. The
  field declaration stays volatile.

Each batch had identical before/after verification of every accepted row.

`exe.host_engine_resolve` is staged masked-equal. It uses localized per-access volatile reads of the
module handle; the original re-reads it from memory at every use. The global stays non-volatile.

Acceptances: 1,083 to **1,101**:

| Change | Count |
|---|---:|
| Local byte tables | 2 |
| Sonnet binding lane bindS3 | 7 |
| EXE indexed form | 2 |
| ENG1 `_g_kind_info` rows (bindK) | 7 |
| ENG3 `_g_kind_info` rows (bindK3, paired anchor) | 34 |
| Re-pinned masked rows bound (bindB) | 2 |

Total: **1,137**.

**`loader-zero-paired-v1`** (`scripts/loader_zero_paired.py`, owner ruling D3) is an alternative
anchor for DLL loader-zero owners that have no absolute base reference. It pins a two-instruction span
outside every compared caller: `mov r32, imm32`, where `imm32 = owner + k`, `k >= 1`, with exactly one
HIGHLOW at the immediate, immediately followed by a scalar load from `[r32 - k]`. The first
instruction always falls through, so executing it loads owner+0. Other control edges into the second
instruction add paths but are not proof inputs.

ENG3 uses 0x1c1b5 `mov edi,0x1003fbd4` with 0x1c1ba `mov eax,[edi-4]`. 0x1c1ba is also a loop head;
its back edge comes from 0x1c275. The owner is the new data-only `src/eng3/kind_info_storage.c`
(`g_kind_info[1]`, 8-byte COMMON, minimum extent only). Tests are in
`tests/test_loader_zero_paired.py`.

**Callee alias renames (reviewed re-pin b10).** 14 units now call accepted callees by their own canonical
symbols. Each old name was an extern-only address label, called directly at the accepted row's entry; Opus
alias review in `work/velocity_i/alias/`. There is no codegen change: re-pin preserved 1,096 rows and 258
masked rows were restaged. Four units whose RAW rows pin unit inventories (handlers_121c0.c,
handlers_28ae0.c, handlers_0e020.cpp, spatial_effects.c) are deferred to a reviewed inventory refresh.
Cross-module twin wrappers, the ENG3 `dbg_printf` twin, TITLE-shared callers and duplicate bodies await
owner decisions.

Accepted: `eng1.native_code_target_011450`, `eng1.3d780` (evidence/bindR_*). Total: **1,139**.
`eng1.3fe00` is held: its `_remove_actor_variant` identity record pins the pre-rename source hash.

New data-only owner `src/eng3/engine_pointer_cells.c` defines four ENG3 loader-zero pointer cells that were
previously only declared extern: `_g_eng3_ranges_4078c`, `_g_eng3_game_40b8c`,
`_g_eng3_middle_actions_40b98` and `_g_eng3_host_40bb4`. Each compiles to an exact 4-byte COMMON, and each has a
legacy anchor outside every row (see evidence/bindP_review_2026_10_31.json). Accepted: `eng3.1b920`. Total:
**1,140**. Four more rows (eng3.03290, 11d80, 25770, 26eb0) are relocation-exact except for
`call [reg+disp]` method calls through those objects. The verifier rejects register-indirect transfers by
design, and changing that needs an owner ruling.

Open:

- **`host_file_open_only_3c50`.** Kept blocked by owner ruling; the historical experiment is recorded in
  `evidence/host_file_open_3c50_history_2026_10_31.json`. Sol's investigation (`work/velocity_i/sol3c50/findings.md`):
  - EAX on the success path is the open result; the empty diagnostic never writes it.
  - The only consumer is the ENG1 wrapper, whose caller discards the value.
  - No exact form free of undefined behaviour was found.
  - It stays blocked pending the owner's source-contract decision.
- **Identities still missing:**
  - 12 ENG1 and 13 ENG3 `_g_kind_info` readers need other identities.
  - `eng1_171b0` needs 5 callees and 3 cells.
  - `host_engine_resolve` and `host_engine_select` need EXE loader-zero cell anchors, imports and CRT
    identities.
  - `archive_open_5130` needs `_archive_normalize_name` and archive cells.
  - `host_setup_slots_reset_a1a0` needs the public file-handle cells.

## Wave j (2026-10-31): autonomous campaign, first integration

The owner delegated routine technical decisions to the integrator. Two opt-in proof forms were added. Each is
dispatched only from the `fixed_disp32.py` comparator copy, and the comparator-preservation test pins the new
hook lines. The pinned modules (`masked.py`, `storage_alias.py`, `zero_storage.py`, `callback_transfer.py`,
`relocated_data.py`) are unchanged, and the legacy comparators still refuse both record types.

- **`opaque-indirect-call-v1`** (`scripts/opaque_call.py`)
  - **What it admits:** one unprefixed near `FF /2` CALL through:
    - a register;
    - register-based memory, with no relocation in the instruction;
    - a loader-zero (BSS) cell whose identity is proven independently.
  - **What it proves:** instruction identity only. Every record states target `unidentified` and abi `unclaimed`.
    The ledger proof scope appends the number of opaque calls.
  - **What stays refused:**
    - JMP, far, prefixed and segment forms;
    - relocated register-based forms;
    - unrecorded register calls.
  - **Independent adversarial review** (`work/velocity_j/opreview/`) found two holes. Neither was used by any row,
    and both are fixed:
    - **D1:** on the relocation-stripped HERCULES.EXE, a literal address cannot be told apart from an original
      address. The form now requires a base-relocation inventory, so it applies to DLLs only.
    - **D2:** a payload-hash `data` record laid over an import slot could authorise a call to the wrong import.
      Cell calls now need BSS records, and rows that opt in refuse `data` records over PE directories or import
      slots.
  - **Bug caught on the way:** the cell check found two masked-equal ENG3 wrappers whose source calls interface
    slot 0x20 where the native code calls slot 0x2ac (`eng3.shared_1ffa0`, `eng3.shared_20120`). They are not
    accepted.
- **`compile-renaming-byte-v1`** (`scripts/storage_alias_byte.py`)
  - Extends the reviewed word alias proof to one-byte scalars.
  - It adds a `/WX` caller-type check, because MSVC5 reports a char-signedness redeclaration only as warning C4142.
  - New headers: `src/eng1/special_kind_alias.h` and `src/eng3/special_kind_alias.h`.

**Accepted:**

- **37 rows through opaque calls** (`evidence/bindQ_*`):
  - the four ENG3 pilots: 03290, 11d80, 25770, 26eb0;
  - the four `create_*_actor` callers: 05ee0, 0e7b0, 0f980, 0fec0;
  - 29 interface-slot wrappers and callers whose slots the fixed-slot review rates conflict or unresolved.
- **2 rows through the byte alias** (`evidence/byteA_*`): `eng1.release_actor`, `eng1.remove_actor_variant`.

Total: **1,179**.

**Planning fix:** `scripts/blocker_queue.py` used to count every absolute-memory call (imports included) as an
unsupported indirect transfer, which hid ready rows. It now counts only transfers that no existing proof form can
admit: 46 rows have no identity blocker, compared with 11 under the old count.

## Re-pin b11 (2026-10-31): wave-j callee reconstructions

Seven units changed through the reviewed re-pin. All 1,140 strict rows were re-verified with identical
findings, and the old sources are preserved as `evidence/repin_snapshots`.

| Unit | Change |
| --- | --- |
| `src/shared/remove_actor.c` | `remove_actor_preserving_resource` and `mark_actor_resource_for_release` added (ENG1 and ENG3) |
| `src/eng1/mode_handlers_16840.cpp` | `eng1_17ff0`; `eng1_17290` (per-read `held_5c` casts, same evidence class as 0x171b0) |
| `src/eng1/actor_handlers_mid.c` | `eng1_26cd0`, `eng1_26de0` and `eng1_26e90` (indexed pre-increment loops) |
| `src/eng1/actor_pair_animation.c` | ENG1 0x1a410 |
| `src/eng1/actor_links.c` | `action_spawn_27d40` |
| `src/eng3/actor_services_1e000.c` | `eng3_1f0a0` and `eng3_1f130` (comparison chains ordered by the C2 node key) |
| `src/shared/bounds.c` | `point_in_box_2a810` |

- **Ledger:** 11 new masked rows were staged in place, and 3 RECONSTRUCTED rows became masked-equal. Every
  masked row in a changed unit was re-staged.
- **Tool:** `repin.py` accepts an `added_functions` list per changed source. A whole-unit alias receipt
  (`functions` count) may then grow by exactly those declared definitions, which must exist in the fresh
  object; every other finding must still be identical.
- **Config:** `config/eng1/sources.json` now selects the matching units for four functions:
  - `_remove_actor_preserving_resource`;
  - `_mark_actor_resource_for_release`;
  - `_action_spawn_27d40`;
  - `_point_in_box_2a810`.

  The earlier behavioural files (`actor_resource_helpers.c`, `action_spawn_27d40.c`, `point_in_box_2a810.c`)
  are no longer selected. They stay in the tree only for the two differential scripts, until the
  duplicate-body retirement.
- **Deferred:** ENG1 0x19b40 is masked-equal in work/velocity_j/solN2, but its unit gains a project header,
  and the re-pin admits only toolchain-header context additions.
