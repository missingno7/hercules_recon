# Level-6 color consumption and lifetime

`26730` now supports level-6 color updates when a pointer list supplies the
color, or at least one of the six effective target/current bytes differs.
It consumes a pending target in native byte order, averages six-byte list rows
without changing the current-color record, or interpolates that record before
calling the existing `5a60 -> 5e00` color implementation. The physical byte
order becomes argument order `0,2,1,3,5,4`. The common camera offset update runs;
the non-level-6 camera angle stages remain skipped.

The two initialized six-byte records at `5a7b0` and `5a7b8` join the existing
color storage provider. The pending pointer at `6fea0` is an interior view of
the existing `g_eng1_spatial_types_06fe60` prefix, at offset `0x40`. A separate
pointer owner failed the data overlap audit and was removed before adoption.
The native physical alias is preserved; original aggregate/TU intent is unproved.

Native producers were identified from instructions and independently queried
through the existing REA client:

- `1ebf0` appends six-byte rows from `58a20`, selected by actor WORD `+32`.
- `1ed30` selects a pending row from `589f8` using the existing transition state.
- `1ac10` initializes target/current to physical bytes `0,0,0,128,128,128`.
- `25d00` consumes the list, then clears count `71ae0`, even at level 9 where
  consumption is skipped. That reset was absent in source and is now restored.

At this checkpoint those producer roles did not claim reconstructed callback
bodies. The [next increment](eng1_color_list_producer.md) supplies bounded `1ebf0`
source; complete producer/resource closure remains open. The reset negative control failed three nonzero-count
cases before the correction; all four reset cases now pass.

## Exact remaining boundary

The native interpolation loop visits eight DWORD stack lanes although only six
are initialized. With six equal real colors and an empty list, changing a low
WORD at input ESP-8 changes whether `5a60` is called. Four controlled Unicorn
cases reproduce `no call, call, no call, call` for DWORD values
`0,1,65536,65535`, without modifying instructions and stopping before `5a60`.
Native-only C stack-poison trials always called it; they did not discriminate
the branch. The controlled emulation establishes the missing dependency.

That equal/empty path remains explicitly unsupported. Source also rejects
negative or out-of-prefix color-list counts and partial pending/target overlap.
All guards run before camera, pending-target or color-state writes. No stack
values or always-call/never-call policy are invented. Full `26730` closure still
requires reconstructing its ambient stack behavior through the real caller.

## Verification

```powershell
python scripts/eng1_bounded_differential.py level6-color --native-eng1 <pinned-ENG1.DLL>
python scripts/eng1_bounded_differential.py camera-chain --native-eng1 <pinned-ENG1.DLL>
python scripts/eng1_actor_differential.py --native-eng1 <pinned-ENG1.DLL>
```

Fresh VC5 passes 101 new color cases/2,538 checks, six source-only guards/30
checks and all 2,484 prior color states: 2,585 total states/38,569 checks. It
compares all 512 colors, complete guarded frame buffers, color records, the
shared spatial prefix, pending consumption, camera offset, ESP and native module
writes. The known fade callback upper-DWORD residue difference remains diagnostic.
The separate camera suite passes 324 cases/6,614 checks and five guards/75 checks.
Actor regression passes 512 prior cases plus four reset cases/56 checks.

All 134 strict rows, 38 source fixtures, 85 Python tests and the expanded
60-provider/929-field data audit pass. The genuine graph remains 369 mapped
entries, 400 function symbols, 529 unresolved symbols, zero duplicate providers
and 76 preserved strict ENG1 functions. No source-built ENG1 DLL is emitted or
game process run, and
`recovery.json` is unchanged. No new exact function match is claimed.

[Behavior evidence](../evidence/eng1_level6_color.json) and
[storage evidence](../evidence/eng1_level6_color_storage.json) pin the proof inputs.
