# Grid/contact caller references, 2026-10-25

Seven unchanged ENG1 contributions gained FUNCTION_MATCH_RELOCATED acceptance:

| Entry | Bytes | Existing source |
| --- | ---: | --- |
| `29da0` | 128 | `calibration/eng1/actor_unit_282e0.cpp` |
| `29e20` | 304 | same |
| `29f50` | 192 | same |
| `ef10` | 144 | `calibration/eng1/actor_effects_0ef10.c` |
| `f0a0` | 224 | same |
| `f3b0` | 128 | `calibration/eng1/level_actors_0c730.c` |
| `3f0b0` | 144 | `calibration/eng1/player_scene_3a040.c` |

The batch adds 1,264 bytes. Recovery.json now contains 415 accepted native entries
/ 40,560 bytes: 136 RAW rows, 236 RELOCATED rows and 43 retained linked rows.
All 408 earlier accepted records remain identical. No canonical function source,
compiler profile, native extent or symbol spelling changes.

The question was whether these callers' reference-only differences could be
closed without pretending the grid/scene helpers were already matching. Their
referenced entries were reviewed from their own native bodies and physical ABIs:

- `20b50`: four DWORD cdecl arguments, with the fourth unused; coordinate/layer
  bounds, two directional scans, real `20990` decoder and packed distance return.
  Its current guarded source compiles to 624 bytes against the observed native
  480-byte span. Its 725 finite native compositions do not grant source matching
  or native row/header/stream capacities.
- `29870`: captured actor coordinates, live-count solid-actor worklist, self
  exclusion, `200000/40000` flag paths, calls to independently accepted `2a220`
  and `2a330`, strict nearest-negative selection and high-WORD result with low
  `7fff`. The own native body identifies the entry independently of the caller's
  implied map. Its sole native caller is in this batch, so no additional external
  call anchor is claimed. Its own source and indexed storage remain unaccepted.
- `295a0`: optional scene anchor, `80000` rejection, `40000` link traversal and
  `e4/e8` insertion writes. Its complete instruction-equal source still has an
  unresolved shared-cell ownership question.
- `29620`: independently RAW-accepted scene unlink entry and pointer ABI.

Full native entry hashes, return/call offsets, absence of indexed external
interior entries and available outside-call anchors are frozen in the reference
review. Fresh caller compilation checks all complete instructions, padding and
every reference operand. Ten wrong-entry controls reject while leaving zero
non-reference differences. Matching callers grants no acceptance to their
callees' source, data owners, source-link closure or runtime completeness.

All 374 strict proofs and 43 legacy linked function checks pass. The 86 existing
broader linked provenance drifts remain unchanged. All 218 Python tests and
1,618 remaining supported masked rows in 199 sources pass. Promotion updates
diagnostic co-defined function inventories as required; it does not change their
code identities or implied maps. The ENG1 source graph's provider inputs remain
unchanged from its 961-provider / 744-unresolved-symbol measurement; no new graph
closure is claimed for status-only acceptance.

A separate private `29870` compiler control reduces ten instruction differences
to three by materializing the captured-y subtraction after wrapped height
accumulation. Simple operand reordering alone did not help. The whole-unit
contribution audit also catches renamed interpreter jump-table locals and actual
byte changes in unaccepted `2a570`. That candidate remains ignored and is not
integrated. Accepted peers and their binding maps remain untouched; the current
canonical `29870` body still has its original ten differences.

Evidence: `evidence/grid_callers_references_2026_10_25.json`,
`evidence/grid_callers_bindings_2026_10_25.json` and the frozen final receipt
`evidence/grid_callers_progress_2026_10_25.json`. Original host/game execution was
not performed. Full matching remains incomplete.
