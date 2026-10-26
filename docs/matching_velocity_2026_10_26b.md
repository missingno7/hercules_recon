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
