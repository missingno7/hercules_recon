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
