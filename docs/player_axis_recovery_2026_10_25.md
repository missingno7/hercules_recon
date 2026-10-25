# Player input and axis recovery, 2026-10-25

Two missing ENG1 services now have real source in
`src/eng1/behavioral/player_axis_motion.c`: X motion at `19a10` (144 bytes) and
Y motion at `19aa0` (160 bytes). Both complete compiler contributions reproduce
every non-reference byte, including padding. They retain signed target comparisons,
DWORD wrap before comparison, asymmetric approach rates, clamp-after-overshoot,
live pool-pointer reloads, position updates and tail boundary continuations.
The X conditional expression reproduces the original condition/value lifetime.

Both were staged as CODEGEN_SIMILAR diagnostics. Their pool-pointer and boundary
references remain unaccepted; no new function matching status is claimed.
The matching ledger remains at 380 unique accepted rows / 34,864 bytes.

The neighboring original-backed mode unit uses WORD input fields at `5c` and `5e`.
The new controller's BYTE tests at `5d` and `5f` select their high halves. Restoring
that shared view in `player_modes_remaining.c` reduces `19480` from 232 to 22
non-reference differences without changing its 464-byte extent. Seven existing
equal bodies remain equal. The remaining differences in that function are two
direction/flags load-order pairs; original source declarations remain unproved.

## Discriminating experiments

Signed motion fields, timer capture, a chained zero assignment and typed pointer
views do not resolve the other residuals. The WORD input layout changes the
result substantially. Explicit direction capture instead changes the flag
combination to XOR and leaves 28 residual bytes; it was not adopted. Ordinary
mask promotion leaves the 22-byte result unchanged. No flags, dummy declarations,
source padding or instruction patches were used. Detailed negative reports stay
under ignored `work/player_mode_context_2026_10_25/`.

The current four residuals are `19260` (6 bytes), `19290` (229), `193f0` (16), and
`19480` (22). All complete extents agree. Nine recovered bodies now have complete
instruction equality apart from unresolved references, covering 896 bytes across
the two new source files. This remains a diagnostic measurement.

## Independent behavior and source closure

The pinned VC5/Unicorn fixture passes 470 original/source cases: the earlier 360
isolated handler cases, 88 isolated axis cases, and 22 composed controller/axis
cases. The composed cases execute the actual recovered axis bodies on both sides.
Other services, including the two boundary continuations, remain modeled.
Comparisons cover ordered writes and service calls, full actor records, shared
cells, callback pointer replacement, signed overflow and target overshoot,
direction transitions, source aliasing and cdecl guards. Neither an original DLL
nor the original game/host process is loaded into the operating system.

The real source graph compiles 946 mapped ENG1 providers, with no duplicate
providers or generated placeholders. Its genuine link still fails with 760
unresolved names: 450 function and 310 data/untyped. The two newly closed services
expose two missing boundary services. There is no source-closure or playable-game
claim. All 380 earlier accepted records remain identical; 339 strict proofs,
1,640 diagnostic rows in 201 sources and all 183 Python tests pass.

## Pool-pointer storage obligation

The source storage provider freshly emits one four-byte COMMON owner for
`g_normal_actor_pool`. Independently, original bootstrap `1f10d` passes slot
`72860` to the pool allocator and reset `274a0` reads that slot before advancing
actors by `0x134`. Existing role/alias evidence corroborates the identity.
The cell lies wholly beyond the section's file-backed bytes and inside its
writable, non-executable virtual extent. The PE section rules specify zero
initialization for that tail. [Microsoft PE format](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#section-table-section-headers)

A same-zero neighboring cell, an oversized owner and a file-backed address all
fail their respective audit controls. The current relocated comparator still
rejects loader-zero storage as unsupported. The audit is evidence for a future
fail-closed storage proof, not permission to skip the existing check, accept
all-zero similarity or infer pool capacity.

Frozen results are in `evidence/player_axis_recovery_2026_10_25.json` and
`evidence/normal_pool_pointer_2026_10_25.json`. Recovery.json alone owns acceptance.
