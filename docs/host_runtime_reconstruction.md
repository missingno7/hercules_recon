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

The byte at EXE RVA `0x3a594` is now labeled as a neutral input gate. Its
literal writers are in the CD/MCI status routine; the previous window-activation
name was unsupported. The rename preserves all97 complete host contributions
and shared storage bytes. Input/config comparisons and all132 accepted matches
pass. See `evidence/host_input_gate_identity.json` for the reference inventory
and source-preservation boundary.

## Dispatcher and system keys

The dispatcher at EXE `0x5ec0` now uses the existing loader/selector providers
and the identified VC5 setjmp/longjmp contributions. Nine bounded comparisons
pass, including live callback changes and nonlocal reentry. Its actual host-table,
callback-byte and jump-buffer owners remain external. The exception-head fixture
uses synthetic `FS:[0] = 0` and zero-backed address zero; real Windows exception
unwinding is unproved.

The original MFC map registers system-key handlers at `0x9ab0` and `0x9b20`
with the three-UINT member signature. Ordinary C++ preserves gate precedence,
the signed threshold, requested mode and pending-input updates. Fourteen cases
pass using the actual display-mode setter and shared providers. Five independent
loader-zero scalar prefixes supply eight observed bytes; their enclosing objects,
relative placement and historical source owners remain unproved. The receiver
declaration is a physical member-call view, not a recovered complete window class.
MFC `CWnd::Default` comes from pinned library member148 and is intercepted at entry;
its internal effects and the complete message loop are outside the comparisons.

```
python scripts/host_dispatch_differential.py --native-exe D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE
python scripts/host_system_keys_differential.py --native-exe D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE
```

All three remain `RECONSTRUCTED`: their complete raw contributions differ.
The same fresh objects are compared, linked and emulated. The fresh regression
preserves all132 accepted functions and the59-provider ENG1 data audit.
See `evidence/host_engine_dispatch_recovery.json` and
`evidence/host_system_keys_recovery.json` for commands, hashes and limits.

## Startup protocol and normal exit

Original EXE `0x5e10` now supplies the timer/service sequence, Config-apply guard,
gate test, exact missing-CD warning and initial requested display mode. The normal
exit at `0x104d0` shares the existing fatal guard, window cell and shutdown-chain
source. It preserves repeated-entry exit(-321), post-cleanup owner reload, WM_CLOSE
notification and exit(0). CRT exit is identified as VC5 library member335 and is
intercepted at entry in the fixture.

```
python scripts/host_startup_differential.py
```

Fourteen bounded comparisons pass with actual shutdown-chain/display-setter
source. Other startup and cleanup service effects are synthetic boundaries. Both
complete instruction shapes agree outside relocation operands, but raw bytes
differ, so both remain `RECONSTRUCTED`. All132 accepted functions remain exact.
The real startup still needs service bodies at `5750`, `5020`, `72a0`, `a0b0`,
`fdf0` and `1ff0`; their declarations do not complete startup. See
`evidence/host_startup_exit_recovery.json` for full proof boundaries and provenance.

## Remaining game work

The current natural host link compiles 75 actual source units covering all 99
recorded EXE functions, preserving its 25 accepted EXE matches. It has no duplicate
providers and stops on 26 missing symbols, including the real host interface,
loader storage, graphics/audio services and six startup service bodies. It produces no executable. The exact
selected sources, pinned libraries and missing consumers are recorded in
`evidence/host_current_source_graph.json`; this is a partial graph, not complete
application coverage.

The recovered pieces still need a coherent host startup chain, window/message
loop, full table ownership and engine dependencies before gameplay can run. The genuine
ENG1 graph has 398 compiled functions and 531 unresolved symbols after the resource
action and color-transition initializer recoveries. A linked diagnostic DLL with fail-on-entry markers remains an
incomplete diagnostic, not a game. Original host execution also retains the
repository's isolation, windowed-startup and display-restoration requirements.
