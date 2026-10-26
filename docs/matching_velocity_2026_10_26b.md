# Matching campaign velocity_b — 26 October 2026 (afternoon)

**Published: 466 STRICT functions / 30,104 bytes**, all FUNCTION_MATCH_RELOCATED.
Baseline `0a22ddead43d97a87c87e98ee4a7d864ca2ce9af` (554 accepted); result
1,020 accepted: 145 RAW, 832 RELOCATED, 43 legacy LINKED.
Every batch went through the journaled promotion gate with fresh full peer
protection and the 43-row LINKED check (its 86 provenance drifts unchanged).
Zero rejected integrations after the one caught protection failure below.

| Batch | Commit | Functions | Bytes | Main identity source |
| --- | --- | ---: | ---: | --- |
| 1 | `957b3a4` | 17 | 1,552 | reused RNG provider; EXE static host table 0x361b8 (file-backed data) |
| 2 | `9d20f28` | 24 | 4,336 | spawn/sound callee rows; `_g_26110` tables; two ENG3 pointer-cell owners |
| 3 | `154afc4` | 22 | 3,424 | interface-root data users; EXE literals; `_g_configuration_dword_713bc` |
| 4 | `07d8df7` | 32 | 4,176 | dispatcher/resource identities; strings; ENG1 scalar cells; ENG3 helpers |
| 5 | `3726d29` | 311 | 6,672 | new `fixed-slot-v1` profile (`8ece9b5`, `ddb6cfb`) |
| 6 | `44c9711` | 48 | 6,312 | ENG3 `_g_focus` 4-byte cell; ENG1/ENG3 scalar cells |
| 7 | this commit | 12 | 3,632 | reuse of identities accepted earlier in the campaign |

## What changed the velocity

1. **Blocker census instead of per-function search.** 1,489 pending rows were
   already instruction-equal (reference-only); none needed compiler steering.
   `scripts/blocker_queue.py` freshly compiles each pending profile (about 180
   compiles, under a minute), classifies every relocation target against the
   ledger and accepted binding documents, and ranks unresolved symbols by the rows
   they unlock. Lanes were cut from that ranking, not from addresses.
2. **One shared blocker, one investigator.** The interface wrappers (339 rows) had
   one recorded NO-GO. A Sol lane established from static bytes that each DLL's
   `PC_DLLEngineMain` copies the EXE table at 0x361b8 at offset zero, that no
   module writes or references the roots or table elsewhere, and that 152 of 153
   slots are physically stack-compatible. Slot +0x4c, the documented conflict,
   stays rejected. A second Sol lane implemented the narrow opt-in profile in an
   isolated worktree. The integrator reused TITLE's accepted COMMON root verbatim,
   repinned the review and moved the dispatch out of the pinned `callback_transfer.py`.
3. **Mechanical lane types**: identity reuse, file-backed data payloads, scalar
   loader-zero cells (minimal exact-size data-only owners with independent HIGHLOW
   anchors: `src/eng1/loader_zero_cells_{1,3}.c`, `src/eng3/eng3_{track,actor}_storage.c`),
   callee identities from own rows or committed native reviews.
4. **Integration without waiting.** 16–22 concurrent `cx` workers (Luna xhigh,
   Sol high for the two shared questions). The integrator promoted ready batches
   while other lanes ran and resumed finished workers with deltas (`--resume`).

## Measured cost

About 2,323 compiler invocations in the opt-in collectors: 2,268 under
`work/velocity_b/compiler_events*` and 55 in the interface worktree. They cover
worker focused checks, the classifier/queue, seven promotion protections plus
two refused attempts, the interface sample check and test runs. That is **20.1
accepted functions / 1,296 bytes per 100 calls end to end**, against 1.20 / 91 in
the previous campaign, on a different task mix. The batch commits span
13:30–14:30 local time (about one hour). Token and cost telemetry are not exposed.

## Protection finding

Accepted rooted-callback proofs pin `scripts/callback_transfer.py` as reference
evidence. The first fixed-slot commit edited it, and the batch-4 promotion
correctly refused (`Independent reference evidence changed`). Keep pinned
verifier files byte-stable; add new profiles in new modules.

## Remaining frontier (ranked by `blocker_queue.py`)

- ENG3 `_g_focus` and `_g_kind_info`, ENG1 `_g_kind_info`: largest data blockers;
  scalar-cell form only where every native access is a direct offset-0 scalar.
- **Indexed BSS** (worklists, `_g_2d2a0`, `_g_fx_rows`, ranges): needs a capacity
  proof, not adjacency. **Same-TU `.bss` statics**: the TITLE same-TU BSS mechanism.
- **Relocated same-unit data** (MFC CRuntimeClass/message maps, 13+ EXE rows) and
  the MSVC duplicate undefined/defined COFF symbol pair (8 EXE deleters): small
  verifier extensions in their own modules.
- Stripped-EXE anchors: `work/velocity_b/exe_anchor_sol/report.json` proposes a
  restricted disp32 memory-operand witness; only about 32 rows qualify. Deferred.
- The ENG1 source graph does not yet select the new data-only owner units; add
  them to `config/` when the next graph build runs.

## Wave c (same day, evening)

**Published: 23 more STRICT functions / 2,688 bytes; 1,043 accepted** (145 RAW,
855 RELOCATED, 43 legacy LINKED). Commits `fd25d3d` (verifier forms), `b494797`,
`8845220`, plus the final pair. About 950 compiler invocations went into 13 Luna lanes,
one Sol investigation and four protection runs: 2.4 functions per 100 calls. That is a
tenth of the first wave's rate. The cheap forms are exhausted.

- One Sol investigator added three opt-in verifier forms in new modules (see
  [verifier forms](verifier_forms_2026_10_26.md)): `relocated-data-v1`,
  `msvc-undef-comdat-v1` and `cmp-test-absolute-v1`. They admitted the eight EXE
  scalar-deleting destructors, one MFC runtime-class accessor and the ENG3 WORD
  mode cell. Same-TU `.bss` statics were **rejected**: VC5 COFF has no
  per-object sizes and next-symbol spacing includes padding (`u0cc90.c`: a 4-byte
  static spans 8). The TITLE menu statics also lie in file-backed `.data`.
- Most remaining rows each need several identities at once. The largest single
  class is direct callees with no ledger row at all (`function_unknown`, ~650 uses).
  Their entries need native identity reviews, so mechanical lanes no longer pay.
- Two protection lessons. Accepted proofs pin `docs/matching.md` (cited by three
  batch-5 documents), so living docs must never be cited as binding evidence.
  Also, an anchor used by an accepted proof blocks acceptance of the function that
  contains it: `eng1.color_profile_5dc0` holds the `_g_frame_global_73488` anchor of
  two accepted rows. `work/velocity_c/anchor_preflight.py` finds such conflicts
  without compiling; run it before every batch.

Next candidates: a reviewed bulk method for native callee identities (entry from
the function map, extent, independent outside callers, ABI) for `function_unknown`
symbols; capacity evidence for indexed BSS; re-anchoring accepted proofs whose
anchors sit inside pending functions (`scripts/reanchor.py`).

## Wave d (same day, night)

**Published: 5 more RELOCATED functions / 928 bytes (ENG3 14f00, 15170, 17090, 26600, 27f10);
1,048 accepted.** About 940 compiler invocations went into 12 Luna lanes, two promotion runs and the
investigators: 0.5 functions per 100 calls. A fresh `blocker_queue.py` run placed zero-section
cells in 643 of 1,000 pending rows; for 229 they are the only blocker.

- Investigator routing: Sol-class tasks went to gpt-6.1-terra first. All three Terra runs returned in
  1–2 minutes with triage only (no compiles, no controls) and were escalated to gpt-6.1-sol.
- Zero-section cells (Sol): unanchored isolation is unsound on original evidence. An ENG3 `REP MOVSD`
  writes 1,000 bytes across 0x407b4 from another base, and cell addresses escape into interface
  callbacks. The existing independent-anchor rule stays.
- Library members (Sol): pinned libcmt/nafxcw sections compared with native bytes. 120 graph
  members close, including CDialog::EndDialog, CWnd::EnableWindow/ShowWindow/SetWindowPos,
  CSliderCtrl::SetRange, `__ftol` and `_strncmp`. No pending row is fully closed yet: compiler
  `$L` labels, MFC vtables and CRT/MFC globals remain. **None of the 14 accepted EXE rows that use
  the 8 weak identities closes**, and operator delete has no unique independent location. The
  owner decides whether to demote them.
- Near-miss and new-callee lanes: one RAW-exact fix (`eng3.initialize_alternate_actor`), and
  instruction-equal `eng3_19f30`, `host_music_apply_1000`, ENG1 0x27410/0x27450 and 0x17ff0. None
  could land.
- **Main structural blocker:** accepted proofs pin canonical source hashes, unit contexts and
  binding `evidence_files` (including src/ files). 973 of 1,195 pending rows live in sources that
  cannot change without breaking a proof. promote.py correctly refused the `actor_pools.c` fix. A
  reviewed re-pin path (fresh peer equality, snapshot-preserved evidence, journaled re-pin) is under
  investigation; until it exists, source-changing lanes cannot pay.
- **Re-pin landed (`1de4a5f`).** `scripts/repin.py` runs a full fresh before/after verification.
  It requires identical complete contributions, relocation records and reference findings for
  every accepted row, and keeps old pinned bytes at `evidence/repin_snapshots/<sha256>.txt`.
  It accepted `eng3.initialize_alternate_actor` (RAW) and the new row `eng3.19f30`
  (RELOCATED); main has 1,050 accepted. One re-pin costs about 370 compiles, so batch many
  reviewed source fixes into each run. Live owner/include/alias pins still need a versioned
  re-proof.
