# Original host runtime reconstruction

This phase recovers the original Windows game in historical C/C++. A new renderer
is deferred. `recovery.json` alone records function recovery; finite comparisons
and relocation similarity do not grant a byte match or runnable gameplay.

## Warm delivery

`calibration/host_warm_delivery.cpp` supplies the original host entries BCF0,
BD30, BD50, BE40 and F820. They connect the optional four-byte input packet,
sound-buffer cursor updates and the existing callback-one delivery provider.
The packet receives the inverted input low WORD and two header bytes. Audio
scans the measured 24 channel slots, respects Config byte +0x0e and uses the
actual 4800-pointer sound-bank owner with the pinned DirectSound interfaces.

State reloads are part of the recovered behavior. Input sampling can change
the packet pointer before its writes. `GetStatus` can change a channel ID before
`GetCurrentPosition` resolves the buffer again. Callback-one can change the
subsequent callback or delivery gate. Ordinary source preserves these cases;
no volatile qualifier or placement control was needed.

Five separate source storage prefixes reproduce the original loader-zero bytes.
The Config, sound-bank and callback-prefix providers remain shared with existing
source. These views do not prove complete original objects, declarations or TUs.

Run the bounded historical comparison with the pinned retail executable:

```
python scripts/host_warm_differential.py --native-exe D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE
```

The 20 original/fresh-source cases pass, including initial zero prefixes, packet
mutation, sound-bank edges, callback mutation and stack/register preservation.
All five complete contributions agree outside ordinary relocation operands;
raw equality fails, so all five remain `RECONSTRUCTED`. The sampler and COM APIs
are synthetic fixture endpoints. The existing callback-one source is compiled
and exercised. No game process, DLL initializer or real input/audio API runs.
See `evidence/host_warm_delivery_recovery.json` for provenance and finite limits.

## Loader, configuration and input

The loader and selector now have ordinary historical C source in separate units.
They preserve basename loading, the three original fallback paths, live path
reloads, missing-module/export behavior and the CRT `longjmp` transfer. The four
CRT dependencies are identified VC5 library contributions. Fifteen finite cases
pass; path/error buffer capacities and the real jump-buffer owner remain open.
The runner's 512-byte buffers are test storage, not adopted game declarations.

The Config readers, defaults, mapping construction and byte setters compile with
the existing checksum, audio, Config and literal providers as nine separate
objects. Their 24 finite cases pass, including the original absent type check,
query-failure key leak, repeated close and twice-applied defaults. Twelve distinct
startup/input storage prefixes reproduce 7,785 native initial bytes. The host
interface table remains an explicit external dependency.

The six input primitives compile with the real SDK imports and shared storage.
Their 15 named cases produce 17 matching output lines and 350 matching API calls,
including complete initialization and buffer/ABI checks. The actual object used
for raw comparison is the same one linked and emulated. Warm delivery now refers
directly to `host_input_collect_8040`; there is no added adapter function.

```
python scripts/host_loader_differential.py --native-exe D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE
python scripts/host_config_differential.py --native-exe D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE --original-root D:/Prog/hercules_recon
python scripts/host_input_differential.py --native-exe D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE
```

These fixtures interpret original and source-built code with synthetic API
endpoints. They preserve all 132 strict matches and add no `FUNCTION_MATCH`.
See `evidence/host_engine_loader_recovery.json`, `evidence/host_config_recovery.json`
and `evidence/host_input_recovery.json` for exact provenance and finite boundaries.

## Remaining game work

The recovered pieces still need a coherent host startup chain, window/message
loop, full table ownership and engine dependencies before gameplay can run. The genuine
ENG1 graph has 398 compiled functions and 531 unresolved symbols after the resource
action and color-transition initializer recoveries. A linked diagnostic DLL with fail-on-entry markers remains an
incomplete diagnostic, not a game. Original host execution also retains the
repository's isolation, windowed-startup and display-restoration requirements.
