# Relocation-free leaf harvest, 2026-10-09

The raw `FUNCTION_MATCH` gate accepts only contributions whose compiled bytes equal
the target with no object or target relocations. Until a natural link reproduces
real addresses, the only directly acceptable code is leaf code: no calls, no
absolute image references and no branches leaving the function. This continuation
measures that pool and reconstructs it. The gate, `promote.py` and the toolchain
are unchanged. Durable record: `evidence/leaf_harvest.json`.

## Survey

`scripts/leaf_survey.py` starts spans at 16-byte-aligned direct-call targets and
code pointers, runs each to the next start, and keeps spans without base
relocations, image references or outgoing branches. It compares each span with every
code contribution of pinned `libcmt.lib` and `nafxcw.lib`; equal bytes are library
code to identify, never to rewrite.

| Leaf spans | Library | Already tracked | Open |
|---:|---:|---:|---:|
| 224 | 83 | 42 | 101 (6,800 bytes) |

Library identifications include the default LIBCMT `DllMain@12` in all three DLLs,
`memset`, `memcmp`, `strncpy`, `strncmp`, `wcslen`, `__aulldiv`/`__aullrem`, `__allmul`,
`__ftol`, `__EH_prolog`, the `__sbh_*` heap and floating-point conversion helpers,
and static `output.c` helpers. These are byte identities of whole contributions,
not proof of an exact library revision.

`scripts/leaf_integrate.py review` checks each extent mechanically: a capstone
decode from the entry covers the body and ends in `ret`/`jmp`, there are fewer than
16 NOP padding bytes, no call or pointer enters the interior, and references from
code are instruction immediates. The archaeology index decodes linearly and can
desynchronize after a jump table, so the review decodes from the entry itself.
All 113 accepted functions pass it. Twenty open spans are switch index tables or
case fragments, and one is a return-address-swapping thunk; none is a function.

## Reconstruction

Prediction recorded before the family: at least 70% of genuine open leaves reproduce
under RTM `/O2` with ordinary C. Falsifier: fewer than 50%, or a need for other flags
or non-ordinary constructs. Six Haiku 5.5 lanes iterated private per-function
candidates with `scripts/leaf.py check`; two Haiku merges unified regions onto the
existing `Effect`/`Actor` offsets. The integrator reviewed every source,
replaced decompiler temporaries with established sibling forms (compound
assignments, `while (count--)` loops, `follow_owner_scale`'s spelling), and staged
rows only after a fresh exact compile of the final file.

The prediction held: 69 of 78 genuine functions are raw exact. Accepted state went
from 44 to 113 functions, adding 3,392 bytes. Four were free: ENG3 copies of accepted
`frame_state.c` functions. New units are `src/eng1/effect_handlers.c`,
`actor_handlers_{low,mid,high}.c`, `planar_point.c`, `src/eng3/actor_handlers.c`,
`src/title/title_handlers.c` and `src/pc/exe_helpers.c`. Grouping follows address
regions and shared layouts. These are not recovered translation units, and names
remain provisional.

## Findings

- **Declaration order steers allocation.** EXE `0x4fe0` was one `lea` operand away
  until `length` was declared before `sum`/`shift` in a `for` loop.
- **Signed constants are distinct.** In the 272-byte reset family (TITLE `0x94c0`,
  ENG1 `0x27fe0` = ENG3 `0x1d8a0`), assigning `-1` to the signed `phase` at 0x32
  reproduces the lone immediate that `0xffff` elsewhere does not share. The rest of
  the schedule remains open; chained assignments compile identically to plain stores.
- **TU context changes code generation.** `0x2a520` is exact standalone or first in
  `bounds.c`. It is 6–8 bytes off after any struct-dereferencing function and 9 off
  directly after its contiguous predecessor `planar_bounds_overlap`, in either order.
  Either a TU boundary lies between them or the predecessor's original spelling
  differs. This is a usable discriminator for TU recovery. Every grouped function is
  verified in place, so region files remain hypotheses.

## Open

Nine genuine leaves remain, each with a recorded family and re-entry condition. They
include the two point-in-box `&` chains (ENG3 `0x1f130`, ENG1 `0x2a810`; all 720
operand orders falsified), the reset family, the backward unrolled copy (EXE
`0x43a0`, 44 bytes, register roles only) and ENG1 `0x2c030` (4 bytes). The leaf pool
is now essentially exhausted. Further acceptance needs the natural-link dependency
work in `docs/handoff.md`.
