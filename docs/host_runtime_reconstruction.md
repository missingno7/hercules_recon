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

## Remaining game work

The original input sampler, configuration/startup chain, window/message loop and
engine dependencies must join this source before gameplay can run. The genuine
ENG1 graph has 397 compiled functions and 532 unresolved symbols after the resource
action recovery. A linked diagnostic DLL with fail-on-entry markers remains an
incomplete diagnostic, not a game. Original host execution also retains the
repository's isolation, windowed-startup and display-restoration requirements.
