# Optional-target steering and nearest-actor query, 2026-10-25

ENG1 `2b5e0` gained FUNCTION_MATCH_RELOCATED acceptance for all 960 contribution
bytes and its independently verified bounds-helper call. The ledger contains
401 accepted functions / 38,176 bytes: 136 RAW, 222 RELOCATED and 43 retained
linked rows. All 400 earlier accepted records remain identical.

Two missing functions now have real source:

- `target_actor_motion.c` reuses the established motion algorithm with an
  optional bounds-centered target and acceleration-only flag stages `40/80`.
- `nearest_actor_2aea0.c` searches the existing count-prefixed `71580` worklist
  using signed-short wrapped deltas, a depth limit and a strict best planar
  estimate. Equal estimates retain the first actor. It remains unaccepted.

## Steering proof

The native null-target path bypasses all three steering blocks directly, then
runs ordinary acceleration and position updates. A flat sequence of masked
flag tests was behaviorally plausible but left 847 non-reference byte differences
in a 960-byte contribution. Expressing the actual branch scope closes every
instruction difference. C++ did not change the earlier residue.

The source consumes the existing bounds header. It preserves mode zero, the
twelve-byte output view, signed-WORD center construction with DWORD wrap,
coordinate fallback when bounds fail, whole-byte direction complements, the
original cross-axis limit comparisons and low-BYTE return. Explicit unsigned
wrap macros for speed/position arithmetic retain exact code while avoiding
signed-overflow assumptions in those expressions. The independently reviewed
`2a880` entry is reused from earlier own-body/ABI evidence; its underlying
resource chain remains bounded and unaccepted. A wrong-entry control rejects
with zero non-reference differences.

## Query boundary and finite validation

All 2,674 isolated original/source cases pass in pinned Unicorn: 1,626 steering
and 1,048 query cases. Steering cases cover every low flag byte, upper argument
bits, null/self targets, bounds success/failure, live callback mutations, signed
extremes and wrap/clamp order. Bounds production is explicitly modeled.

Query cases use signed zero/negative counts and synthetic worklists of zero to
eight entries, strict ties, signed-short absolute-value overflow, threshold
edges and seeded input states. The source still has 190 non-reference byte
differences. Its compiler can reuse incoming cdecl argument-copy slots; those
copy values are excluded from this finite ABI comparison. Return address, ESP,
callee-saved registers, caller guards, returned actor, complete heap and worklist
effects are checked. Inputs do not alias those argument copies. These cases are
not RAW equality or universal equivalence.

The query uses the existing aggregate interior view, without introducing a
second worklist owner or a guessed capacity. The aggregate/prefix evidence does
not prove complete native table ownership. Neither this storage boundary nor
the query's compiler differences are relaxed for acceptance.

Final promotion checks all 360 strict proofs and 43 legacy linked functions;
the existing 86 broader provenance drifts remain unchanged. All 218 Python tests
and 1,627 supported diagnostic rows in 199 sources pass. The genuine ENG1
source graph has 955 mapped providers out of 1,231 diagnostic spans, with zero
duplicates/generated placeholders and 750 unresolved names (440 function,
310 data/untyped). It still does not link. Matching, finite component behavior,
source coverage and whole-game completeness remain distinct; no original host
or game process was launched.

Evidence is in `evidence/spatial_queries_recovery_2026_10_25.json`,
`evidence/target_actor_motion_reference_2026_10_25.json` and
`evidence/target_actor_motion_bindings_2026_10_25.json`. Final counts and report
hashes are in `evidence/spatial_queries_progress_2026_10_25.json`.
Recovery.json alone grants acceptance.
