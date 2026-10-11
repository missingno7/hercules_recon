# TITLE behavioral primitives

This private lane assembles the six missing implementations listed below from
the existing candidate drafts. It is a source-level behavioral contribution;
no raw-code equality or `recovery.json` status is claimed. The reproducible
assembly is `assemble.py`, its private builder config is `build_config.json`,
and `run_build.py` invokes the ordinary TITLE behavioral builder.

| RVA | Behavior | Source provenance |
| --- | --- | --- |
| `0x3100` | Emit unrotated, unscaled sprite quads into the active ordering table. | `candidates/wm4/d3100v2.c`, following the completed emitter family draft; `TitleCell` and `TitlePrim` are the existing same-TU views. |
| `0x3420` | Emit unscaled, rotated sprite quads from the object's angle and sine table. | `candidates/wm4/best3420.c`. |
| `0x3740` | Scale sprite offsets and dimensions, then emit unrotated quads. | `candidates/wm4/best3740.c`. |
| `0x39b0` | Scale and rotate sprite quads, then traverse linked atlas cells. | `candidates/wm4/d39b0v4.c`, the selected 2026-10-10 draft. |
| `0x4fe0` | Initialize 256 sprite-cell masks and texture-page selectors, reserving four cells. | `candidates/wr4/r04_none.c`; the existing `title_data.c` provider supplies the 256-cell storage. |
| `0x54f0` | Resolve an effect ID, calculate owner-based panning/volume, request a voice, and attach a successful voice. | `candidates/wm6/best.c`; the existing `r04d80.c` `TitleRec64` view and `title_fx_pan` helper are reused. |

The emitter fragment is appended to `calibration/title/u02b70.c`, so the private
`g_29d90` / `g_29d94` cell allocator state, `TitlePrim`, and `TitleCell` view
remain in that translation unit. The initializer and sound function are
appended to `calibration/title/r04d80.c`; the sound fragment adds only the five
private cells used by the draft (`g_29da8`, `g_29db0`, `g_29db4`, `g_29dc0`,
and `g_29e00`) and leaves the pre-existing `g_29dac`, `g_29db8`, and `g_29e04`
definitions where they are.

The only input adaptations correct the three emitter prototypes to the draft's
byte count and signed scale arguments, and make the existing `title_03050`
definition agree with its already declared stream-pointer/byte-count contract.
The sine-table declaration points at its existing data-unit provider. The cell
initializer uses the same twelve-byte record layout in its own TU; no duplicate
storage is declared.

Remaining assumptions: sprite-cell storage capacity is the observed 256-cell
prefix, not a proved original aggregate extent; exact source names and original
type declarations are unknown; scale arguments are signed fixed-point values
as inferred from the neighboring dispatcher; the host sound API and runtime
render/audio results have not been exercised here. A successful fresh compile
or natural link establishes only that this behavioral source graph builds.

Fresh build on 2026-10-11: both extended units compiled with pinned
`msvc5_rtm` through `scripts/match.py`; all six added symbols are present in the
builder report and there are no duplicate providers. The initial private link
experiment stopped on 17 dependencies elsewhere in the graph. The complete
behavioral build now supplies those providers and produces a DLL with zero
unresolved symbols. The linked initializer also passed 64 randomized native
oracle comparisons, including untouched fields; renderer and audio behavior
remain unverified. See `docs/title_behavioral_differential.md` and the current
`work/title_behavioral/build/report.json`.
