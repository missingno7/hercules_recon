# VC5 RTM compiler and linker findings (2026-10-20)

Reusable knowledge measured on the pinned VC5 RTM toolchain while reconstructing Hercules.
Each entry says how it was established and how confident it is. "Masked equal" is a diagnostic
milestone, never strict acceptance. Sister projects (blupi_recon, blood2_recon) traced their
C2 rules on SP3; treat their rules as hypotheses here unless re-measured below.

## Established (reproduced by controlled builds)

| Finding | Evidence | Use |
|---|---|---|
| Engine sound/demo unit is compiled `cl /c /Od` as C **without /Gy**; ENG3 is the same source with one preprocessor switch | 36/36 functions masked equal (ENG1 0x5ea0..0x83d8, ENG3 0xae00..0xd27f); the whole non-/Gy `.text` contribution is masked equal as one block in both modules; /Ge, /Gf, /O1, /O2, /Os, /TP and /Gy rejected with reasons (`evidence/od_sound_unit_2026_10_20/flags.md`) | Unaligned frame-pointer functions packed back to back signal an /Od unit; segment it as one contribution and compile with /Od |
| /Od stack locals are ordered by identifier hash, not declaration order | 60/60 random-name probes: frame filled top-down in ascending 16-bucket order of the `scripts/symbol_order.py` hash (`& 0xf`); later-defined first within a bucket; temporaries last | Layout-chosen local names (owner ruling 2026-10-20) may reproduce an observed frame; document them; they do not recover original names |
| .bss within one object: uninitialized statics first in identifier-hash order, then explicit-zero objects in definition order | toy objects (work/tdata `probe_ez.py`) | Explains unit .bss layouts; explicit `= 0` definitions are placed after statics |
| .data: an object's initialized globals form one section in definition order before its string literals; all-zero initialized objects go to .bss; partial initializers keep their zero tail in .data | toy objects | Ownership of unreferenced initialized blocks follows literal adjacency |
| LINK 5.00 communals: owner = first command-line object with a tentative definition; groups in reverse command-line order; library communals after all object groups; within a group ascending by identifier hash bucket; alignment min(32, next power of two >= size) | toy links (work/tdata); TITLE's 92 referenced communals fall on 4 deltas at 4 unexplained gaps | Natural-link data placement |
| Library code in all four images is VC5 **RTM** LIBCMT/NAFXCW (+ CTL3D32S in the EXE) | every member of RTM/SP2/SP3/VC6 libraries matched with relocation masking; RTM covers strictly more (`evidence/library_attribution_2026_10_20.json`) | Pin RTM libraries for natural links |
| A 999-sentinel loop's sound value in the original takes the "named local" register route (eax, no scratch-rotor advance); an expression temp advances the rotor | minimal pairs (`evidence/rotor_mechanism_2026_10_20.json`) | Explains rotor-phase residues; no admissible natural source form found yet |
| Switch case order can decide a join reload (TITLE 0xa450: equal only when case 0xffff precedes case 4) | 30 placements measured | Contradicts the SP3-derived "case order is inert" for this RTM decision |
| Base/index order (SIB) is a declaration-count tie-break in **C and C++** | ENG1 spawn_kind/spawn_normal/change_kind/pool reset equal from 77/82/161/37 extra declarations in C; C++ at lower counts | A SIB residue does not discriminate C from C++ |
| Operand order of commutative sums/OR chains follows expression keys that include front-end symbol ids (e.g. g_engine_interface's id selects the TITLE 0x1c500 level-switch order) | lane tJ c2 traces (unpinned SP3 tooling, diagnostic), id-phase scans | Source spelling of such terms is inert; only context changes it |
| /Od negation `0 - (x + 20)` compiles to xor/sub; `-(x + 20)` gives neg | probe (`negprobe.c`) | Spelling discriminator in unoptimized units |

## Hypotheses that still need independent evidence

Declaration-count/TU-context effects are real compiler mechanisms, but a header that reaches the
needed count phase is **not** evidence of the original include (owner guidance 2026-10-20). Units
that currently carry a header only as count context, each to be re-decided with independent
evidence (genuine use of the header, or the unit's real extent):

- `calibration/title/u197f0.c`: `<windows.h>` + `<memory.h>` (memory.h stands for any +1..+8 mod 16 id shift)
- `calibration/eng1/actor_unit_282e0.cpp`: `<stdlib.h>`/`<string.h>`
- `calibration/eng1/render_unit_30010.cpp`: `<string.h>`
- `calibration/eng3/handlers_28ae0.c`: `WIN32_LEAN_AND_MEAN` + `<windows.h>`
- `calibration/actor_factories.c`, `calibration/actor_pool_reset_repaired.c`, `calibration/eng3/actor_middle_pool_driver.c`: `<windows.h>`
- earlier TITLE units (u01de0.c, u02b70.c, u06350.c) from the 2026-10-09 ruling

Delivered **without** the count context (rows stay RECONSTRUCTED until independent evidence appears):

- `calibration/eng3/handlers_10370.c` 0x10590 (commutative add destination; lean windows.h equalises,
  0..63 extra externs do not) and 0x10df0 (which `random_limit` call is evaluated first; 9..17, 24
  or 48 leading declarations equalise). Same mechanism as y4's 0x292f0/0x28ae0/0x297d0.

## Negative results (do not repeat without new evidence)

Value/pointer locals, casts, literal suffixes, OR-operand order, inline helpers, prototype forms
and loop shapes are inert for the 999-loop rotor residue; `unsigned short` callee parameter does
not change it. Statement/declaration orders, types and C++ are inert for the TITLE 0x164b0
scheduling residue; `/Oa` reproduces it but breaks every reload. The 0x29990..0x29d90 TITLE .bss
window is not a table read by 0xd910 (the 0x29c2c read is a folded displacement).
