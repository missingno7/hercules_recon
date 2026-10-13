# Lock, unlock, and presentation closure

## Prediction and falsifier (recorded before this family's checks)

Prediction: the host lock helper either returns a status value ignored by the
known frame/movie callers or provides the active surface view through the
published pointer/pitch cells; the unlock and presenter then consume that same
view while retaining the original DirectDraw and GDI objects. In DD mode,
lost-surface recovery and a failed lock may leave a different set of published
cells than the successful lock. In DIB mode, pitch is fixed at 320 and the
presenter reads the DIB allocation. The writeback/coherence contract for an
independent renderer is not established until exact field writes, status
branches, and CPU writes are checked.

Falsifier: any reviewed caller branches on the lock result as a success gate;
the successful DD path does not publish the returned pointer/pitch; failure
continues with a stale view; present consumes a separate or transformed view;
unlock mutates a published field unexpectedly; or CPU writeback is not visible
to the original DIB/DirectDraw consumer under the observed call sequence.

## Findings

The pinned routines are the unchanged HERCULES image spans below. The lock and
unlock helper signatures take no explicit arguments; neither consumes a
caller-supplied surface. Their COM calls pass the `this` pointer as a stack
argument, consistent with the observed DirectDraw vtable byte offsets in
`renderer_contract/evidence/presentation.json`.

| Host cell or service | Proven role |
|---|---|
| `BA020C` | Nonzero selects the DirectDraw path; zero selects DIB/GDI. |
| `BA0210` | Raw 32-bit address of the currently published writable pixel view. |
| `BA0214` | Byte pitch. DD mode copies `DDSURFACEDESC.lPitch`; DIB mode writes 320. |
| `BA01F0` | Lock-state byte. Set after pointer and pitch publication; cleared by unlock. |
| `43A590` | Present gate. It is cleared before present attempts a service call. |
| `96E3E8` / `96E3EC` | Primary surface / attached backbuffer. |
| `96E3D8` / `96E3F4` / `96E400` | DIB pixel pointer / `BITMAPINFO` / HDC used by GDI presentation. |

The mode cell is read as a byte at the bounded service sites: `6FF0+0x11`
(`mov al, byte ptr [0xBA020C]`), `7140+0x09` (the same AL byte load), and
`6620+0x2B` (`mov dl, byte ptr [0xBA020C]`). The proposal therefore uses an
8-bit mode snapshot field, not a DWORD view of that cell.

In DIB mode `6FF0` publishes `96E3D8`, pitch `0x140` (320), then sets
`BA01F0=1`. In DirectDraw mode it calls `IsLost` on the primary at vtable
offset `0x60`; exactly `DDERR_SURFACELOST` (`0x887601c2`) causes `Restore` at
`0x6c`. The restore result is ignored and the host still tries `Lock` on the
attached backbuffer at `0x64`, with a null rectangle, `DDLOCK_WAIT` (`1`), and
null event. A successful Lock publishes `lPitch` to `BA0214`, then `lpSurface`
to `BA0210`, and finally sets `BA01F0=1`. The synthetic spy confirmed this
order and the vtable arguments, including with a non-320 pitch.

Two edge cases matter to an adapter. If the attached backbuffer pointer is
null, `6FF0` publishes zero pitch and a null pixel address but still sets the
lock-state byte. If `Lock` returns a failing HRESULT, the routine does not
publish those fields or set the lock byte; it calls `6EC0(0)` to clear the
requested-mode byte at `96E3D0`, then enters the `6BB0` mode transition. The
fixture set `96E40C=1`. At `6BB0`, the exact guard is `mov eax,[96E40C]` at
RVA `0x6BB0`, followed by `test eax,eax` and `jne 0x406EB9` at `0x6BB7`; the
branch lands directly on the helper's `ret` at `0x6EB9`, before `push esi` or
any window/display work. This proves only that the initial failure route calls
`6EC0(0)` and that the guarded transition preserves the preloaded
pixel/pitch/lock cells. It does not prove the unguarded fullscreen exit or
Windows/GDI behavior. On a surface-loss result, `Restore` failure alone does not prevent
the Lock attempt. The direct-call movie path checks `BA01F0` after `6FF0` and
then validates `BA0210`; the helper's EAX is not a reliable success result.

Unlock `7140` is also argumentless. If `BA01F0` is zero it returns. Otherwise,
when DD mode is selected and the attached backbuffer exists, it invokes
`Unlock(backbuffer, NULL)` at vtable byte offset `0x80`; it clears `BA01F0`
afterward regardless of HRESULT. In DIB mode or with no backbuffer it simply
clears the byte. It leaves `BA0210` and `BA0214` unchanged, so treat that view
as valid only while the lock byte is set and reacquire it for each later lock.

Presenter `6620` is argumentless and first tests `43A590`. When set, it clears
the byte before checking the presentation path. With HDC and DIB globals
available, DD mode calls `Flip(primary, NULL, DDFLIP_WAIT)` at vtable byte
offset `0x2c`; it does not consult the lock pointer/pitch. A missing primary
or failing Flip still leaves the dirty byte clear. Windowed mode calls
`StretchDIBits` with the original HDC, dimensions from `BA0220/BA0224`, source
size 320x240, `96E3D8` pixels, and `96E3F4` bitmap info, then calls `GdiFlush`.
Those return values are ignored. If the HDC or DIB pointer is missing it takes
the host HWND fill path when an HWND exists. The 10-case receipt exercised the
successful DIB call and the DD missing-primary/failing-Flip paths with spies;
no OS or graphics API was invoked.

The return register is not a status ABI. Direct callers do not branch on EAX:
the movie caller checks the published lock byte, and the other reviewed lock
callers proceed from the global view. Synthetic lock success returned the
supplied pitch (`0x150`) in DD mode and preserved an arbitrary incoming EAX
(`0xA5A50000`) in DIB mode; an already-locked entry preserved
`0xA5A50001`. Unlock and present callers also ignore EAX; the COM HRESULT or
`GdiFlush` value can simply survive the tail. A new adapter must key off the
published cells and host call order, not infer a Boolean from these returns.

## CPU writeback and ownership boundary

The host's renderers write directly through `BA0210 + y*BA0214 + x` while the
lock is held. In windowed mode that view aliases the DIB pointer later passed
to `StretchDIBits`; in fullscreen it is the backbuffer address returned by
DirectDraw Lock, and `Unlock` followed by Flip exposes that surface. The
presenter performs no copy from a private renderer buffer. An independent CPU
renderer that retains these services therefore needs to copy final indexed
bytes into the currently locked view using its published pitch before unlock;
it must not assume pitch 320 in DD mode or keep the pointer after unlock. A
null view with a set lock byte must be rejected before writing.

The existing AC60 tail also remaps the active view through `70C0(0xff)` before
unlock/present, while the movie callback locks, copies borrowed movie bytes
using the returned pitch, sets `43A590=1`, unlocks, and presents. Palette and
window-message handlers can clear, update palettes, and present through the
same original surface/DIB. The presenter itself only runs when the dirty gate
is set. The dirty byte's initial file value is 1, and it is reset by present;
later writes include the movie and palette/message routes, while `6EE0` marks
its clear operation dirty. No proof here establishes that every independent
renderer callback will set the dirty byte or that movie/palette/clear writers
are serialized against it. That callback/frame scheduling and ownership remain
an integration blocker.

## Prediction result and reproduction

The prediction above held for the reviewed lock callers: the pixel view is
published through globals, and callers do not consume a lock status return.
One falsifier did occur in the isolated failure fixture: a failing Lock left
the prior pointer/pitch stale while the transition guard prevented the full
mode switch. The actual unguarded `6BB0` path can alter those cells through its
mode transition, so the stale-view result is limited to the bounded initial
failure path. The backbuffer-null path instead publishes a null/zero view but
sets lock state. The separate original AC60/movie and palette paths remain a
write-ownership conflict to resolve at integration time.

## Reproduction

The shared [surface header](hercules_surface.h) is a tool-side snapshot, not a
native aggregate. The [native receipt](evidence/native_surface_contract.json)
pins the original spans, runner, header and synthetic service events.

```powershell
$py = 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $py -B renderer_contract/tests/test_original_surface_contract.py --repo-root . --exe D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE --output work/replay/surface.json
```

This executes unchanged original instructions only in Unicorn. The Lock-failure
fixture sets the transition guard at 96E40C, so 6BB0 takes its early return at
6EB9. It proves the preceding requested-mode clear and retained view cells in
that guarded case, not the full display/window transition.
