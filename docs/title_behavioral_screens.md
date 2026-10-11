# TITLE animation and options source append

This candidate appends definitions for four target functions to the existing `calibration/title/u09a40.c` unit through the behavioral builder's `unit_extensions` mechanism. The append reuses that unit's frame tables and same-TU private state; it does not add stubs, replace original-binary behavior, or modify `recovery.json`.

| RVA | Function | Reused draft | Contract |
|---|---|---|---|
| `0x9a40` | `title_09a40` | `candidates/h9a40/base.c` | Advances the `g_2a1e8` animation and its shared phase handoff. |
| `0x9c90` | `title_09c90` | `candidates/wr4/pi_9c90_110.c` | Advances four frame/sound table sequences and updates the shared phase/object state. |
| `0xa020` | `title_0a020` | `candidates/wr4/pi_a020_11.c` | Advances the third frame/sound sequence and its object state. |
| `0xa450` | `title_0a450` | `candidates/wa450/u09a40_ctx.c` | `TitleProc` options callback: asset load, object setup, input/settings handling, and process cleanup. |

The selected bodies are ordinary C and retain the drafts' state transitions, table accesses, callback order, and update order. New static cells are the missing private state visible in the 0xa450 draft plus three cursors used by the animation functions. `title_slots.h` supplies the existing 20-byte `g_26110` record view; `title_files.h` supplies the screen-file list; `TitleOptionsView` preserves the observed context offsets.

Helper declarations use existing source-unit evidence: `title_05e90`/`title_05f10`/`title_07510` follow the process API in `calibration/title/title_proc.cpp`; `title_0c3f0`, `title_0c450`, `title_0c6d0`, `title_0c8b0`, and related wrapper returns follow `calibration/title/u0c1f0.c` and existing screen callers. `title_054f0` retains the object-pointer view used by the animation source draft. The pointer-return view for `title_17ad0` follows its use as a `TitleObject *` in screen source; its existing wrapper source uses an integer return, which has the same x86 return register but is a remaining source-type distinction.

Fresh compile: VC5 RTM from `C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953`, `/O2 /Gy`, with the original unit directory on the include path. Compilation succeeded with no warnings and emitted all four assigned symbols plus the unit's existing `_title_0a300` and `_title_0c140`. See `work/title_behavioral/screens/compile_report.json` and `compile.log`.

The original inventory places the spans at 0x9a40/0x9c90/0xa020/0xa450 with nominal extents 592/912/736/7408 bytes; 0x9c90 and 0xa020 are undecoded leads, so these are not accepted extents. The canonical fragment is now `src/title/behavioral/screens.inc` and all four handlers link in the complete DLL. Their animations and options interactions remain behaviorally unverified until runtime tests exercise these paths.
