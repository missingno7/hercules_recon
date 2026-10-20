# /Od region: compiler-flag verification (ENG1 0x5ea0..0x83e0, ENG3 0xae00..0xd280)

Toolchain: pinned VC5 RTM `msvc500-8abf95ce…` (`scripts/match.py` DEFAULT_TOOLCHAIN); SP2 checked too.
All results are relocation-masked diagnostics (`scripts/masked.py`), not FUNCTION_MATCH.

## Verified flag set

`cl /c /Od` (C, not /TP), **no /Gy** in the original object; ENG3 = same source with one
preprocessor switch (`/DENGINE3`, name invented). Equivalent spellings: no optimization flag
at all (VC5 default is /Od).

Evidence:
- Probe file `flagtest.c` (0x6613, 0x8117, 0x827f, 0x83d3; ordinary C, first attempt) is masked
  equal in ENG1 and ENG3 under /Od. Full matrix in `flagmatrix.json`.
- Whole region `eng1_sound_od.c`: 18/18 ENG1 and 18/18 ENG3 functions masked equal under /Od
  (via compile_source, i.e. /Od /Gy per-function COMDATs).
- Whole object compiled WITHOUT /Gy (`nogy.py`, same env): a single `.text`
  (IMAGE_SCN_ALIGN_16BYTES) holding the 18 functions back to back, 0x2538 bytes (ENG1) /
  0x247f bytes (ENG3 with /DENGINE3). Masked-compared as one contribution at 0x5ea0 / 0xae00:
  equal, 0 differing bytes, 0 conflicts, no two symbols implying one address (`symbols.json`).

## Why other flags are rejected / not discriminated

| Flags | Result | Reason |
|---|---|---|
| /Od, (none), /Od /Oy-, /Od /Oy, /Od /Gd | equal | /Od already implies frame pointer, cdecl default |
| SP2 /Od | equal (18/18) | RTM vs SP2 not distinguished by this code |
| /Od /Zi, /Od /Z7 | equal, whole unit equal | debug info does not change /Od code |
| /Od /Gs, /G5, /GX, /Op, /Ob1, /Oi, /MD | equal | no code shape in this TU depends on them |
| /Od /Ge | differs | `__chkstk` call in every prologue with locals (0x6613: 95 vs 88 bytes) |
| /Od /Gf | per-function equal, **unit conflict** | pools the duplicate literals "\n RemoveFxLinks(0x%x)" (0x4db54 and 0x4db88) and "\n RemoveFxIdLinks(%d,0x%x)" (0x4db6c / 0x4dba0) into one symbol; the original has separate copies. Also rules out /O1 /O2 for the strings |
| /O1, /O2, /Os, /Od /Og | differ | no frame pointer / registers cache globals; /O2 adds NOP alignment |
| /Od /TP (C++) | unit differs from ~0xdbe on, .text 0x2550 bytes | C++ codegen differs (cf. VC5 /TP base/index note); TU is C |
| /Od /Gz | n/a | stdcall decoration; callers clean the stack (add esp,N), so cdecl |
| /Od **/Gy** | functions equal, **layout wrong** | /Od COMDATs carry IMAGE_SCN_ALIGN_16BYTES, so a /Gy link would start every function on a 16-byte boundary with INT3 fill; the oracle packs all 18 functions with no padding (unaligned starts such as 0x6005, 0x60f9, 0x75e3) and only pads after the last one |

## Prologue / epilogue facts reproduced by /Od

- `push ebp; mov ebp,esp`; frame `sub esp,N`, or `push ecx` when the frame is exactly 4 bytes
  (0x6da0, 0x7cde, 0x7ef6, 0x8117, 0x82b7); no frame when there are no locals (0x827f, 0x83d3).
- Epilogue `mov esp,ebp; pop ebp; ret` when a frame exists, else `pop ebp; ret`; never `leave`.
- `push ebx`/`pop ebx` only where an expression needs a fourth register (0x6da0, 0x75e3:
  `rows[ch].scale = ctx->sound[idx][2]`).
- No stack probes. Every global/pointer is re-read at each use. Switch: jump table and byte
  index table emitted inline after the function's `ret` (0x6432/0x644a; ENG3 0xb2d9/0xb2f1).

## Contribution layout

ENG1 0x5ea0..0x83d8 (+8 INT3 to 0x83e0), ENG3 0xae00..0xd27f (+1 INT3 to 0xd280): one
16-aligned non-COMDAT `.text` each, preceded by an /O2 function whose NOP alignment ends at
0x5e9f / 0xadff. INT3 after the last function is linker inter-contribution fill.

## /Od local-variable frame order (new, needed for exactness)

Locals are NOT laid out by declaration order. Fitted 60/60 on random-name probes
(`localfit.py` seeds 1, 2) plus `localprobe.py`: locals are placed from the frame top downward
in ascending 16-bucket order of the identifier hash used for .bss layout
(`scripts/symbol_order.py` hash, folded, `& 0xf` instead of `& 0x3ff`); within one bucket the
later-defined local is placed higher. Compiler temporaries (switch value) come after named
locals. `localnames.py` picks names satisfying an observed order; names chosen this way in
`eng1_sound_od.c` are layout-compatible names, not recovered identifiers:
`offset/clip` (0x646c), `entry, leftvol, rightvol, info, playing, i` all bucket 9 (0x666b,
defined in reverse), `play_channel, snd_bank, fxid, snd_sample, fx_rec, def_volume`
(0x75e3), `snd_bank, i, [fxid,] snd_sample` (0x7d67, 0x7f8e, 0x819a), `path/handle` (0x8330).
