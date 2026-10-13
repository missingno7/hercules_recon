# Bounded host bootstrap-order audit

## Result

The pinned host's **direct initialization path** calls `DirectDrawCreate`:
`0x7937 -> 0x5e10 -> 0x72a0`, then `0x72a0` calls palette setup at `0x71a0`
(`0x741a`) and the import thunk at `0x7426`. A separate event/tick routine
chain is `0x79a0 -> 0x79fc -> 0x5f20 -> 0x5ec0`; if the loaded export pointer
is non-null, `0x5ec0` reads the current table pointer at `0x5ef3` and calls
the loaded engine export at `0x5f03`, passing the table pointer. The event/tick chain itself does not call
`DirectDrawCreate` or the initialization routine.

The DirectDraw result does **not** gate continuation. At `0x742b`, the host
tests `EAX == 0`: zero skips an error log, and nonzero takes the log path. Both
paths rejoin at `0x743c` and continue with DirectDraw-object calls. Neither
`0x72a0` nor `0x5e10` returns early on a failed result. A selected-table clone
that must affect any later cold engine-entry copy therefore cannot be
published only for HRESULT zero; it must be available before the later
`0x5ef3` pointer read for either returned result. Publishing after the
forwarded import returns covers the direct initialization path. It may be too
late if the separate event callback is invoked reentrantly or before
initialization completes.

This does **not** prove a process-wide temporal guarantee. There are no direct
`.text` callers of event callback entry `0x79a0`; its address occurs as a
file-backed code-address literal at `.rdata` RVA `0x2e240`, consistent with an
indirect dispatch route. That callback's direct routine chain reaches the
guarded export call without traversing `DirectDrawCreate`. Whether the
framework invokes it before or after initialization, or re-enters while the
forwarded import is active, is unresolved. So the static evidence does not
rule out a cold table copy before the DirectDraw call returns or on an
independent path that never calls it. Keep publication before the first
possible engine dispatch; a strict post-return guarantee needs
caller/reentrancy evidence.

## Bounded CFG facts

- `0x5e10` calls graphics setup at `0x5e2c` on every normal path to its return.
  It then calls `0x7bb0`, `0xa0b0`, `0xfdf0`, `0x1ff0`, performs host setup,
  and calls `0x6ec0`; it does not call the engine dispatcher. Its only direct
  `.text` caller in the complete raw-section sweep is `0x7937`.
- `0x72a0` calls `0x71a0` at `0x741a` and the DirectDraw thunk at `0x7426` on
  every normal path to its return. Every allocation/palette/COM-result branch
  in the bounded body rejoins a later setup step; those checks select logging
  or skip logging rather than an HRESULT-based early return. The pinned CFG
  includes all direct conditional/jump edges for both routines.
- `0x5ec0` first calls the module-resolution path at `0x5ee5` only when
  `[0x91c460]` is nonzero, reloads the export pointer from `[0x91c45c]`, and
  skips both the pointer read and indirect call when that pointer is null.
  When nonnull, it reads `[0x4365a0]` at `0x5ef3`, reads the byte at
  `[0xba01fc]`, and makes the indirect call at `0x5f03`.
- The only direct calls to `0x5ec0` in the complete `.text` candidate sweep are
  `0x5f97` and `0x5fc5`; the only direct caller of `0x5f20` is `0x79fc`. The
  `0x79a0` callback reaches `0x79fc` only when `[0xba0238] != 0` and
  `[0xba0228] == 0`. `0x5f20`'s accumulator gates its first dispatch at
  `0x5f8c`, can make up to two such iterations while subtracting `0x21`, then
  clears `[0xba01fc]` at `0x5fb3` and makes a final dispatch only when the
  remaining accumulator is nonnegative.
- A full raw `.text` linear sweep with Capstone skipdata covers 183,808 bytes
  (65,375 decoded instructions and four skipdata bytes). This is a direct
  reference inventory, not proof that every decoded byte is instruction
  aligned. Direct callers: `0x5e10 <- 0x7937`, `0x72a0 <- 0x5e2c`,
  `0x71a0 <- 0x741a`, `0x5f20 <- 0x79fc`, `0x5ec0 <- {0x5f97, 0x5fc5}`.
  `0x5f03` is an indirect `call eax`, so it has no direct call-site xref.

## Prediction and falsifier

**Prediction:** on the direct initialization path, any later export dispatch
uses the table pointer read after `DirectDrawCreate` returns to `0x72a0`,
regardless of whether the returned result is zero or nonzero. A clone
published only on zero result will not preserve the same table selection on
the nonzero continuation path. The separate callback route could dispatch
earlier; its invocation order is unknown.

**Falsifier:** reject this direct-order result if a normal path through the
reviewed initialization chain returns before traversing `0x7426`, or if another direct caller of
`0x5ec0`/`0x5f20` is found in a corrected full-section instruction inventory.
Static evidence alone cannot resolve whether the indirect framework callback
route runs before or after initialization or re-enters an API; those remain
explicit unknowns.

## Evidence and reproduction

[bootstrap_order.json](evidence/bootstrap_order.json) pins the original HERCULES.EXE SHA-256, selected original span
hashes, full-section scan coverage, direct reference lists, and bounded CFG
edges. The executable identity is
`587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`.

Run from the renderer checkout root:

```powershell
python renderer_contract/tests/audit_bootstrap_order.py --original-root . --exe D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE --output work/replay/bootstrap_order.json
```

The audit is read-only with respect to the original image and writes only its
JSON receipt under `work/`. It uses the checkout's existing `scripts/pe.py`,
`scripts/env.py`, and Capstone dependency.

The later [gate audit](bootstrap_gates.md) establishes zero-filled initial
state for the event gates and export pointer. The ordinary direct init route
publishes that export after DirectDraw returns. It does not recover the indirect
gate setters or exclude reentrant/alias routes, so process-wide first-copy
ordering remains unresolved.
