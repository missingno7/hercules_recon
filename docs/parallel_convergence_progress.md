# Parallel convergence increment, 2026-10-11

Two new ENG3 functions add **128 relocation-masked code bytes**. The TITLE lanes
produced reusable compiler discriminators but no target matches. The historical
compiler and strict acceptance rules are unchanged. Research used GPT-6.1 Sol
high in disjoint private lanes; one integrator installed the measured ENG3 source.

## Measured source gain

`calibration/eng3/counted_dispatch.c` reconstructs the twins at `0x13a0` and
`0x13e0`, each 63 instruction bytes plus one natural NOP. Each rereads a signed
count after its callback, selects the next actor from a count-prefixed vector,
and dispatches through the unsigned byte at actor offset `0x97`.

The prior source selected `cursor[1]` and then advanced the cursor. VC5 biased the
initial cursor by four bytes and emitted `[cursor]`, leaving 30 residual bytes.
A fresh strict verification of the shared Hercules `remove_counted_value`
routine supplied an independent raw-word count/entry representation anchor.
The new family selected through `*++cursor`, matching the observed advancement
before the callback. VC5 then retains the count-word base and emits the native
`[cursor+4]` load followed by the cursor increment. The typed pointer control
also matches; a raw-word postincrement control still fails. This result is narrow
evidence about these kernels, not a general pointer-optimization rule.

The cleaned source removes the unused struct view and introduces no storage
definitions, capacity claims or placement controls. Two fresh compiles reproduce
complete contributions and noncode output identities; integrator staging compiles
again. Both extents match every non-relocation byte, all four HIGHLOW offsets
`[1,19,43,48]`, and coherent vector/callback bindings. Raw comparison still differs
by 16 address bytes per function. They are `CODEGEN_SIMILAR`, never
`FUNCTION_MATCH`. Evidence: `evidence/eng3_counted_dispatch.json`.

A bounded whole-contribution fingerprint search across indexed aligned entries
in the four existing PC images found only these two twins. It does not exclude
unindexed functions or related functions with another shape. Original names,
source identity, macro ancestry, TU, capacities and callback table contents remain
unknown. No additional original binary was needed.

## Compiler research retained

**Join rematerialization.** The Sol lane constructed two 464-byte controls with
the same outer mapping: ESI=0, EBX=15, EDI=1. When all incoming paths kill those
values, the join restores 1,15,0. When one incoming path preserves zero, the join
places its zero restoration first and lets that path branch past it. Thus edge
availability can force a skippable prefix independently of final register rank.
Both control outputs reproduce in fresh compiles. TITLE `0xa450` still differs
by six residual bytes; the inspected incoming paths require all three values to
be restored, so the mechanism has not been transferred. Preserve the best draft
and its two equal unit peers. Next discriminator: a nested-switch literal-piece
control retaining the target allocation, or genuine source/type/TU evidence.

**Decimal graph partition.** Cut-down tests separate units-pointer promotion
from negative-100 lowering. Removing only the units' second `+90` use recovers
its late load and value's EBX allocation, but leaves the negative multiply chain.
Scalar controls show that a remainder feeding only the next quotient uses a
positive 100 product; adding the terminal units remainder changes it negative.
That occurs without sprite pointers or saved-register pressure. PSX secondary
evidence justified independent place-value products. They recover the positive
chain but keep extra product ranges; full candidates remain 240 versus 224 bytes.
An ordinary split units store folds back to the unchanged baseline. Five decisive
outputs reproduced in fresh builds. No decimal source was installed.

A separate compiler fingerprint control used the pinned driver's documented
`/Os` versus default `/Ot` preference, with all other inputs fixed. The size
variant changes instruction selection substantially and fails the native full
gate; the scalar positive/negative partition persists. Speed-versus-size costs
therefore do not explain the native graph in these controls. No production option
was changed. Next discriminator must explain shared positive intermediate
remainders without retaining the independent product ranges.

Private reports are under `work/sol_join_11` and `work/sol_decimal_11`; compact
durable identities and reentry conditions are in
`evidence/parallel_convergence_2026_10_11.json`. Do not replay the failed families.

## Blood II and historical compiler evidence

The inspected Blood II reference now reports HEAD
`00a56bd63ab4d155c28efb2a9a9d38be3a0fde18`, newer than the previous audit's
snapshot. Seven selected files match that HEAD after newline normalization;
this is not a clean-worktree claim. A cumulative codegen map labels examples
proven that current compact knowledge still labels unproven. Those examples
were excluded as accepted controls. Its ICL pointer-to-array result remains a
representation hypothesis, superseded here by the accepted Hercules anchor.
The useful transfer remains controlled experiments, current evidence checks and
joint context validation, rather than SP3/ICL conclusions.

The local RTM driver/front ends/back end match their pinned hashes. Archived
Microsoft [VC5 language notes](https://jeffpar.github.io/kbarchive/kb/165/Q165684/)
and [known-problem index](https://jeffpar.github.io/kbarchive/kb/168/Q168912/)
did not supply an applicable integer rematerialization mechanism. The inspected
[floating-point issue](https://ftp.zx.net.nz/pub/archive/ftp.microsoft.com/MISC/KB/en-us/170/514.HTM)
concerns a different operation and C++ class context. None justifies changing
Hercules's compiler. File/blob/tool hashes and source URLs are retained in
`work/sol_blood2_research_11` and the compact durable research evidence.

## Validation

Fresh gates pass **113 strict rows / 7,840 bytes**, and **344 masked rows /
81,312 bytes in 31 sources**. Relative to the previous checkpoint, the strict
gain is zero and the masked gain is two functions / 128 bytes. All **54 Python
tests** pass; all **12 immutable originals** verify. The derived coverage overlay
now removes the closed ENG3 count-vector members from pending codegen work.
No assembly, arbitrary placement, extra binaries or weaker verification was used.
