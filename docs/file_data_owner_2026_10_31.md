# The file-data-owner-v1 proof form (31 October 2026)

This opt-in form proves references from a compared function to extern initialized,
file-backed DLL data. It extends the [matching proof rules](matching.md) without
changing that file: accepted proofs pin `docs/matching.md` byte for byte, and
`docs/` cannot be snapshotted. Implementation: `scripts/file_data_owner.py`, with
dispatch hooks in `scripts/fixed_disp32.py` and `scripts/match.py`, a preflight
deferral in `scripts/reference_preflight.py` and anchor refresh in
`scripts/reanchor.py`. Tests: `tests/test_file_data_owner.py`,
`tests/test_file_data_review.py`, `tests/test_file_data_preflight.py`, and the
file-data cases in `tests/test_reanchor.py` and `tests/test_fixed_disp32.py`.

## Record and claim

A symbol record has `kind: data`, `data_profile: file-data-owner-v1`, `rva`, `size`,
the native payload `sha256`, `owner: {source, flags, symbol}`, `evidence`,
`references` and `file_data: {extent_claim: minimum, anchor: {rva, size, operand,
sha256}}`. `payload_bindings` is allowed only for a relocation-bearing owner payload.
The storage claim is exactly this: the owner object's complete storage, compared
byte for byte with the original file bytes at `[rva, rva+size)`. No size, end,
capacity, element count or C type of the original object is proved, and runtime
writes are not ruled out. The schema value `minimum` only records that no end is
claimed. Complete extents, end bounds and one-past uses are refused.

## What is verified

`match.py` freshly compiles the pinned C data-only owner under `src/`. Its source and
include context must be pinned in the binding's `evidence_files`. The owner must
define exactly one strong initialized object of the caller's COFF name, outside
COMDAT, BSS and code, with no functions, weak symbols, debug sections or
non-defaultlib directives. The object must be the last one in its section. VC5
measurements: `short tbl[3]` followed by a `long` leaves `_tbl` at 0 and the long
at 8, so a following object can hide two padding bytes. Alone, or last after a
`long`, the section ends exactly at the object (sizes 6, 10 and 9 for the probes).
Its size must equal the reviewed `size`, and its bytes must equal the original file
bytes.

Relocation-free storage may contain no original loader fixup in `[rva-3, rva+size)`.
Relocation-bearing storage uses `relocated_data.verify_payload` on the owner object,
with the comparator's shared `payload_identities` seeded with the compared function.
Every pointer child must therefore agree with the caller's own records, as on the
legacy same-unit path. The original HIGHLOW inventory must equal the owner's DIR32
inventory, and payload data children must be defined by the owner.

The start address comes from one complete original instruction outside the compared
caller and the selected batch. It is a `mov`/`push` imm32 or a direct disp32
operand of `mov`/`push`/`lea`/`movsx`/`movzx`/`cmp`/`test`, holding
`image_base + rva`, with exactly one original HIGHLOW fixup on that field. Host acceptance status is never consulted.
After the comparator, `match.py` refuses an anchor host whose reference binding in
the same run binds the anchor field to another symbol or addend. It also refuses one
that names the start address, or storage covering it, under another symbol, or binds
this symbol elsewhere. Direct caller accesses must lie inside `[rva, rva+size)`.
Indexed uses bound only their displacement. Stripped images, TLS or load-config
effects and PE data directories are refused. The legacy comparators refuse the
profile, and combining it with a `coff-local-byte-table-v1` table is refused.

## Review history

An adversarial review of the solDF package
(`work/velocity_j/dfreview/test_df_review.py`) found two holes, now fixed and
covered by `tests/test_file_data_review.py`.

- **F1:** payload children were verified against an empty identity map, so one
  binding could put `_callee` at two addresses, or rebind the compared function.
- **F2:** inter-object padding counted as object storage.

The package also had to stop editing the pinned `docs/matching.md`.

## Scheduling and re-anchoring

Verification excludes only the compared caller and the selected batch, so a host's
promotion makes the anchor inside it unusable while that host is selected. Integrator
policy is to schedule a symbol only with a stable witness or with at least three
distinct host rows that name the cell consistently. RECONSTRUCTED hosts must be
confirmed by a fresh compile. Before its host is promoted, a proof is re-anchored
with `scripts/reanchor.py`. That tool now also moves `file_data.anchor` and changes
nothing else in the binding.

## Pilot

`eng3.24f50` references `_g_eng3_scale_39518`, a 16-byte vector at 0x39518 owned by
`src/eng3/render_scale_39518.c`. The anchor is `push 0x10039518` at 0x20c19 in
RECONSTRUCTED `eng3.eng3_20b60`. Alternates are at 0x2134b (`eng3.eng3_21200`) and
at 0x24cbc and 0x24e3e (`eng3.eng3_24bb0`). Fresh host compiles name the cell at the
exact field in the first two hosts. In `eng3_24bb0` they name it with the same two
references, shifted by four bytes. Through the final package, the references gate
gave `RELOCATED EXACT`: 240 bytes, 4 verified references, no non-reference
differences, and 2 compiler calls. Evidence: `evidence/file_data_24f50_review_2026_10_31.json`
and `evidence/file_data_24f50_bindings_2026_10_31.json`.
