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
