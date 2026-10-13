# Gate 3 phase-19 probe readiness

The compact hash-bound evidence index is
[`evidence/presentation_forwarding.json`](evidence/presentation_forwarding.json).

## Result and evidence boundary

Status: **source-built post-entry controller passed; original-host forwarding
remains unresolved; live original gate blocked for root review**. This lane
launched no Hercules process and no original engine DLL. The already-recorded
original pre-entry receipt still establishes only sibling `ddraw.dll` selection
and clone publication before the EXE entry instruction; this work did not
repeat that claim or continue beyond that stop.

The private candidate extends only
[`fixtures/presentation_probe`](renderer_contract/fixtures/presentation_probe)
and carries the existing
[`fixtures/preentry_probe`](renderer_contract/fixtures/preentry_probe) beside
it. It uses the 15bf8ab renderer checkout, the repository's manifest-pinned
`msvc5_rtm` toolchain
(`C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953`, commit
`8abf95ce980161ad87b0b02402269cce76988953`, archive SHA-256
`5f7728d1b3b8b8c61bb35e42a7680211019d290b30376c1a0a8faaa6d166cd1f`), and
the checkout's existing ABI clone utility. No tool was added or downloaded.

The fresh synthetic receipt is
[`presentation_probe_receipt.json`](synthetic_rawabi5/presentation_probe_receipt.json)
and its detailed helper record is
[`build_receipt.json`](synthetic_rawabi5/synthetic/build_receipt.json). The
source-built 32-bit target changed a synthetic host pointer cell from its
source table to a stable 250-DWORD clone, preserved all 248 unselected words
and the source table, dispatched slot 81 once and slot 89 six times, and
recorded 18 committed trace events with zero drops. Slot 81's original integer
return and both DirectDraw fake-backend HRESULT/object/argument paths matched.
The private x86 call harness observed slot-81 EAX `0x13572468` and direct
slot-89 EAX `0x89ABCDEF`; its mask `0x1f` passed for Cdecl stack cleanup and
EBX/ESI/EDI/EBP preservation on both wrappers. Nested slot 89 returned
`0xA89F00D1`, and four concurrent calls preserved distinct return values
`0x90000000` through `0x90000003`. All six slot-89 returns were recorded after
the original call and before leaving the wrapper. One deliberate nested
callback reached maximum depth two; the concurrent calls completed with
thread IDs and complete event records. The target then held after printing its
full passing trace; the harness timed out, terminated it, drained output, and
verified the process was reaped with a nonzero exit code. Synthetic
self-module and self-export guards also passed. The pinned VC5 RTM compile had
zero warnings. These are source-built
ABI/copy/trace/cleanup results only; they do not verify an original DLL call.

The new source-built debugger-controller receipt is
[`postentry_synthetic_receipt.json`](postentry_synthetic_run7/postentry_synthetic_receipt.json)
(SHA-256 `bd3c06cbc9b74d950a4bcdbda81adc280b33c3d2e09e1798f753a7b5df381959`).
It pins 22 fixture/support inputs and the VC5 RTM toolchain, records the
controller/host/engine/proxy hashes before and after execution, and reports
zero compiler warnings. Positive, cold/warm, invalid-signature, and timeout
synthetic cases passed; the `--original` case returned `BLOCKED` and
`ORIGINAL_TARGET=NOTRUN`. It verifies the debugger event-drain and reap path
for source-built children only.

The copied pre-entry builder passed its build-only synthetic controller tests
at [`build_receipt.json`](preentry_synthetic/build_receipt.json). The record
shows the debugger's normal/failure cleanup paths, kill-on-debugger-exit, and
pending-event/exit-event drainage against a source-built target. Its `--host`
mode was not invoked. Neither synthetic fixture establishes original-host
cleanup behavior.

The existing first raw real-system API receipt remains unchanged with status
`UNEXPECTED_FORWARD_MODULE_PATH`. The separately accepted source-built
DirectDraw create/release receipt is still only a standalone diagnostic-process
result, not forwarding from Hercules. No same-process result is promoted to
host integration. Original host forwarding, engine initialization, windowed
startup, callback dispatch, frame display, and normal shutdown remain
unobserved for this candidate.

## Static cold-copy observation recipe

The static [interface-publication audit](../../../renderer_contract/interface_publication.md)
identifies the original cold copy in each pinned engine's `PC_DLLEngineMain`:

| Original module | Expected SHA-256 | 250-word destination RVA | `rep movsd` RVA | Expected post-copy EIP |
|---|---|---:|---:|---:|
| ENG1.DLL | `01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757` | `0x70f20` | `0x2c19f` | `0x2c1a1` |
| ENG3.DLL | `aefa1147deae55c4cb2290282f5142df00baa49a9cd59f671f7a3ce5b8b7fca7` | `0x407a0` | `0x1f84f` | `0x1f851` |
| TITLE.DLL | `b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b` | `0x2bb40` | `0xc20f` | `0xc211` |

An original-module controller must record and verify the loaded engine path
and hash before observing its table. A write watchpoint on slot89 can fire
while the 250-word copy is still in progress; it is not a completion marker.
The candidate design uses an execution breakpoint at an independently pinned
post-copy EIP first, then validates all 250 local words against the published
clone and checks the unchanged source table. Any later slot89 data watch is a
clobber detector. Thread creation must receive the same applicable debug
register setup. Unknown module identity, wrong EIP, partial copy or mismatched
unselected words is a failed observation.

Likewise, stopping just after the wrapper's indirect call proves the original
callee returned. The wrapper's post-log, raw-EAX restoration and return to its
caller must complete before claiming a returned-wrapper observation. These
are distinct observation stages; neither stage proves displayed pixels.

The private synthetic post-entry controller now exercises the analogous
source-built path. It resolves exported native engine marker RVAs from PE
exports, confirms those markers are in executable sections, and sets per
thread DR0/DR1/DR2 execution breakpoints without instruction patches. At the
cold marker it verifies all 250 copied words against the published clone and
checks both replacement slots and the unchanged source table. At the
post-callback marker it checks the completed slot-81/89 trace, including
slot-89's raw physical EAX while its public semantic type remains `void`, then
terminates the synthetic child and drains its exit event. Positive,
cold/warm, invalid-signature, timeout, and hard-blocked-original modes pass in
the source-built receipt. This closes only the synthetic controller
implementation check. The original-module watchpoint recipe above is still
unimplemented, and no original engine or Hercules process was run.

## Exact live-gate blockers

1. **Explicit root review is still required.** The attachment limits this
   worker to preparation and synthetic verification. The prior pre-entry
   authorization ended at the entry breakpoint; it does not authorize any
   later host or engine execution.
2. **Host variant identity must be fixed before any launch.** Retail EXE SHA-256
   is `587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`;
   the distinct no-CD copy is
   `b3ab3ee9e5c973bb245d8eed795e8e7065d7dfc371e412e602b2059af894a551` and
   differs at file offset `0x94b0` / RVA `0xa0b0`. The proxy's timestamp,
   base, and size checks do not distinguish those files. The external
   controller must hash the exact disposable host image before startup and
   verify its loaded path; it must not patch or overwrite either original.
3. **Original module identity must be verified at load.** Keep all game DLLs
   original and verify their full hashes/path before arming a table watch.
   These are the expected hashes: ENG1
   `01b78ae814892007bb6031b2470f10e09bdd912fdb8d30de9f4cfea7c638f757`, ENG3
   `aefa1147deae55c4cb2290282f5142df00baa49a9cd59f671f7a3ce5b8b7fca7`, TITLE
   `b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b`.
   A matching basename or table signature alone is not identity.
4. **System-forwarding identity needs an in-process gate.** The proxy builds its
   requested path from `GetSystemDirectoryA`, rejects its own module and
   `DirectDrawCreate` export, and calls the resolved export outside DllMain.
   The earlier standalone system API candidate recorded the physical
   SysWOW64 file, mapped PE identity, HRESULT, object, and Release in one
   process; it did not prove the original host used that object. A future
   controller must validate the actual loaded system module's mapped identity
   and physical file/hash from its load event before allowing the call to
   proceed. The preserved first raw path receipt must retain its original
   failed logical-path status.
5. **Registry/profile and writable paths are not isolated.**
   `docs/eng1_integration.md` documents separate source-built ENG1 experiments,
   including a no-CD one-byte change to a disposable EXE copy, but it gives no
   complete registry, save, or config path inventory. The presentation
   contract separately warns that install paths can come from registry and
   writes can escape the working directory. No disposable profile/registry
   view or comprehensive user-writable-path rollback has been demonstrated.
   Resolve and redirect those paths in a disposable environment, preserve
   existing user state, and verify the rollback before any launch. Copying the
   EXE to `work/` is insufficient. Do not use the source-built replacement
   ENG1 from the integration experiment for this gate.
6. **Windowed startup is unproved.** The normal host route initially requests
   fullscreen. Identify a supported disposable windowed configuration and
   prove requested/current mode bytes are zero plus the expected caption,
   menu, and client size before accepting a callback. Abort on fullscreen or
   unverified display restoration. This probe does not suppress host display
   services.
7. **The original post-entry boundary remains unproved.** The source-built
   controller now verifies image-load path/PE-size checks, per-thread hardware
   breakpoints, cold and warm 250-word copy checks, one returned synthetic
   callback pair, invalid-signature rejection, timeout, kill-on-debugger-exit,
   pending-event drainage, and child reaping. It does not observe original
   module file identities, their table-copy instructions, a windowed host
   callback, or original-host shutdown. The original launch gate therefore
   remains blocked; no gameplay input, fullscreen route, or frame-seeking loop
   is part of this work.

Even after these gates, passive slot-81/89 forwarding will not give ownership
of AC60/direct texture/VRAM paths, TITLE movie slot 148, slot-149 text, GDI
paint, palette/LUT, surface, window-message, mode-transition or presenter
writers. Slot 149 remains a separate `TextOutA` route: the windowed branch
uses `GetDC(NULL)` to obtain the desktop DC, then calls `ReleaseDC` with the
current HWND and same HDC; fullscreen calls the attached surface vtable. Its
trigger and visibility timing remain unresolved. The host still owns the HWND
and its 320x240 indexed presenter. Adding native-resolution output at the
original HWND would still
require ownership of host mode writes and the final surface/present path. A
GPU final-present path would also need a defined boundary for texture-memory
versus CPU-visible framebuffer state, palette/LUT conversion, and the other
writers listed above. Leaving the legacy presenter active creates read/write
and ordering races even if the GPU only submits a final image. Shutdown must
restore the original display mode and release only resources whose ownership
was transferred. This audit does not choose between those designs and
contains no GPU, presenter, or gameplay implementation.
