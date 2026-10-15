# Frame pacing: does a blocking present change game speed?

**Answer (static, PC originals).** Mostly no. The simulation runs on a wall-clock
fixed step of 33 ms (about 30.3 steps/s), not on presented frames. Each `0x5F20`
tick presents at most once and catches up with up to two non-presenting steps.
A backend that blocks on vsync inside the slot-89 bracket therefore changes
**which steps are shown**, not how many steps run. That holds while one
presenting tick (CPU work plus the block) stays at or below about 67 ms. The one
evidenced coupling is the clock reset: palette refresh `0x69A0`, CD play `0x9E60`
and mode sync `0x5A50` discard elapsed time. If a reset happened on every frame,
step rate would follow the host loop, and therefore vblank waits. The engines'
static palette call sites are setup-time only.

Every fact below is re-derived from bytes by
[audit_frame_pacing.py](tests/audit_frame_pacing.py) (34/34 checks); the receipt
is [frame_pacing.json](evidence/frame_pacing.json). Inputs: HERCULES.EXE
`587ae2e9…`, ENG1 `01b78ae8…`, ENG3 `aefa1147…`, TITLE `b17a37fb…`. The game was
not run.

**Prediction and falsifier.** After a first reading of the `0x5F20` bytes, the
prediction was a wall-clock fixed step with render-skipping catch-up and
presentation only on the last step of a tick. It would have been falsified by any
of these:

- a second clock or wait in the engines or in the tick path;
- presentation on flagged steps;
- a VSync-style host slot (248) that waits or returns wall-clock time;
- `BA01FC` or clock-cell writers outside `0x5F20`;
- asynchronous callbacks into the engines.

None was found. The clock-reset coupling was not predicted.

## The tick

The idle owner ([callback_metadata.md](callback_metadata.md)) is `CHerculesApp::OnIdle` `0x79A0`. It calls
`0x5A50` and `0x6BB0`. When `BA0238 != 0 && BA0228 == 0`, it then calls `0x5F20`,
then `0x6BB0` and `0x5A50` again. Otherwise it calls `WaitMessage`. It returns
`CWinApp::OnIdle(0)`, which is `0 < 1`, i.e. TRUE. So `CWinThread::Run` spins
OnIdle with no sleep while the game is active. Neither side call reaches `Sleep`.

`0x5F20` is byte-identical to the RECONSTRUCTED oracle
([host_timing_dispatch_5f20.c](../calibration/host_timing_dispatch_5f20.c),
body SHA-256 `2da2abca…`). Its decoded constants are clamp `0x42`, step `0x21` and
loop bound `2`:

1. Call `0x9CD0` (CD-track timer; see below), then `timeGetTime`.
2. `acc += now - prev`. A missing previous sample or a nonzero reset cell
   `91C46C` sets `acc = 0` instead. Clamp `acc` to ±66.
3. While `acc >= 33`, up to two times: set `BA01FC = 1`, call `0x5EC0` (one
   engine step), then `acc -= 33`.
4. Clear `BA01FC`. If `acc >= 0`, set `acc -= 33` and call `0x5EC0` once more.
   This step presents.

`91C470` and `91C474` are referenced only inside `0x5F20`. `BA01FC` is written
only there and read by `0x5EC0` and AC60.

The time base is `timeGetTime` alone. EXE imports include `timeGetTime`,
`timeBeginPeriod`/`timeEndPeriod` and `Sleep`. There is no QueryPerformanceCounter,
GetTickCount, `timeSetEvent`, `WaitFor*` or waitable timer. `0x5E10` (end of
InitInstance) requests 1 ms resolution and the shutdown chain `0x5E90` releases
it; the movie stream nests a second request. ENG1, ENG3 and TITLE import no clock
or wait API, and no such name appears anywhere in their bytes.

## What a catch-up step skips

`0x5EC0` calls `PC_DLLEngineMain(table, BA01FC)`. All three modules store
`arg2 & 1` into context `+0x40`. In ENG1's dispatcher `0x20300`, a nonzero `+0x40`
skips exactly two calls: `0x32BD0` and `0x2DA90`. The restart block clears it.
Everything else still runs on a flagged step: the full simulation iteration,
slot-89 submission, the frame toggle and the `+0x38` tick. The semantics of
`0x32BD0` and `0x2DA90` are not established, and ENG3/TITLE consumers of `+0x40`
are not enumerated.

In AC60 (slot 89), `BA01FC = 1` has these effects:

- It skips the color fill `0x6EE0` and the surface lock `0x6FF0`.
- It skips class `0x20` and `0x60` primitives. Their walk continues and state
  classes still apply.
- At the tail, it skips the LUT `0x70C0(0xFF)`, unlock `0x7140` and present
  `0x6620`. `0xBD80` runs on both paths.

So flagged steps advance the game without drawing or presenting.

## Where the original already blocks

- `0x6620` presents with `Flip(NULL, DDFLIP_WAIT)` (vtbl `0x2C`). `0x6FF0` locks
  the back buffer with `DDLOCK_WAIT` (vtbl `0x64`). The GDI path uses
  `StretchDIBits` plus `GdiFlush` and has no vblank wait. The original DirectDraw
  path was therefore already vsync-coupled inside this bracket. How long `Flip`
  or `Lock` actually blocks depends on the driver and the flip chain (two back
  buffers requested, falling back to one;
  [presentation.md](presentation.md)), which bytes cannot show.
- `0x69A0` palette refresh sets the reset cell, then unlocks, clears and presents.
  In DirectDraw mode it then calls `WaitForVerticalBlank` (vtbl `0x58`) with flags
  1, 4, 1 (BLOCKBEGIN, BLOCKEND, BLOCKBEGIN). Its callers are:
  - slot 145 `0x5760`: the palette-file branch calls `0x69A0(1)` at `0x57DA`, and
    the preset switch at `0x59EF`. Preset indices 0 and 4, and out-of-range
    indices, return without a refresh.
  - slot 148 movie `0x5450`.
  - the mode transition `0x6BB0`.
  - table init `0x71A0`.
- The movie slot 148 runs its own loop paced by `timeGetTime` and `Sleep` (`0x37A0`)
  with a stream thread at `0x33E0`. That thread references none of the clock or
  flag cells and calls no engine-callback invoker.
- `0x9CD0`, called at every tick, replays the CD track through `0x9E60` when it
  ends. That path calls `Sleep(100)` on MCI failure and then sets the reset cell,
  so a stall there causes no catch-up burst.
- Slot 248 (the engines' VSync-style call, ENG1 wrapper `0x2CC30`) only
  increments and returns a counter. It never waits. The callbacks set through
  slots 92 and 249 are invoked synchronously, from AC60's `0xBD80` and from
  `0x40BD21`, never by a timer or thread.

## Model (constants from bytes, timing assumed)

The receipt's `model` section replays the `0x5F20` arithmetic. Its assumptions:
no re-entry, free idle iterations, and a present that returns at the next vblank
after its CPU work.

- **Fixed host period T per tick.** Full speed holds for every T ≤ 67 ms.
  Above that, the ±66 clamp starts discarding time (68 ms gives 0.999, 80 ms
  0.96, 120 ms 0.82), apart from isolated exact periods at 69, 77 and 99 ms.
  Presents fall to 1000/T while flagged steps fill the gap.
- **Vsync-blocking present, 50–144 Hz, 2–20 ms CPU.** Speed is 1.000 in every
  case. At 60 Hz, presents lock to 30.0/s, and about 1% of steps become
  non-presented catch-up steps because 33 ms is shorter than two refreshes
  (33.3 ms). That is a frame-delivery judder, not a speed change. Half-rate
  displays (30 Hz) behave the same way; 20 Hz still runs full speed at
  20 presents/s.
- **Worst case: reset on every present.** This is not observed per frame. Speed
  becomes refresh/60.6 when each reset pays the `0x69A0` waits: 0.99 at 60 Hz,
  1.24 at 75 Hz, 1.98 at 120 Hz. With no wait (the GDI branch, or a non-blocking
  replacement), speed is unbounded. Here a backend's blocking would change game
  speed.

## Requirements for a native backend in the slot-89 bracket

- Blocking on vsync (or a swap-chain present) inside the unflagged step is
  compatible. Keep one presenting tick at 67 ms or less.
- Present nothing on flagged steps (`BA01FC = 1`), and keep AC60's gates.
- Keep the `91C46C` reset writes in `0x69A0`, `0x9E60` and `0x5A50`. If palette
  slot 145 or the movie path is replaced, keep its blocking character: at least
  one vblank per call in fullscreen. A replacement that neither waits nor resets
  would change speed only if those slots were called per frame.
- Do not move presentation outside the `0x5F20` tick. Doing so would decouple
  it from the flagged/unflagged choice.

## Limits

- This is static evidence only. Driver flip-queue behavior, actual `Flip`/`Lock`
  durations, real host-loop periods, and whether `timeGetTime` honors 1 ms on
  modern Windows are unobserved.
- Function bounds come from a recursive-descent walk without following indirect
  jumps; slot 145's switch path was checked separately. Callees reached through
  the engine export, which is an indirect call, are not walked from `0x5F20`.
- Slot-145 frequency is inferred from direct call sites only: ENG1 one, in the
  dispatcher's state-0 block `0x203F6`; ENG3 one; TITLE seven, with preset
  indices or paths. Whether any TITLE site repeats per frame was not traced.
- Other host slots that reach `timeGetTime` were not checked for returning time
  values to the engines. Examples are the CD slots 53, 54 and 208, through
  `0x9E60`.
- The model ignores callback re-entry and cell mutation by callbacks. The
  reconstruction notes that the clock state stays live across them.
