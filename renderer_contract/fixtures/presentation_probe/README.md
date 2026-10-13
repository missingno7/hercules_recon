# Phase-19 passive interception candidate

This is a private, source-only candidate derived from the bounded presentation
fixture. It is not installed as `ddraw.dll` and has not been run with
Hercules. The default build exercises only source-built x86 targets and the
source-built synthetic DirectDraw backend.

The candidate keeps the app-local import proxy and process-lifetime 250-DWORD
clone. Its two stable callbacks wrap table slots 81 (`cdecl int(void *,
DWORD)`) and 89 (`cdecl void(void *)`), then synchronously call the saved
original function. Slot 89 stays semantically `void` in the public host ABI;
the candidate uses a private physical x86 `cdecl uint32` call type so it can
capture the raw EAX immediately and return those same bits after recording.
The diagnostic does not change the public callback declaration. No packet
pointers are traversed. Event slots are reserved with Interlocked operations
and marked committed only after all fields are written. A reader must suspend
every target thread after callback completion and reject any uncommitted
record or trace overflow. Nested callbacks are allowed; they add interleaved
enter/return records without a lock. The published table and trace storage
stay in the proxy module for the process lifetime.

`ProbePublishSyntheticTable` uses the same clone routine as the DLL attach path
but applies it to a source-built host table and a synthetic pointer cell. The
target dispatches through that published cell, checks all 248 untouched words,
the original table, and both callback argument/return paths. The x86 call
helpers compare raw EAX, Cdecl stack cleanup, EBX/ESI/EDI/EBP preservation, one
nested slot-89 return (`0xA89F00D1`), a direct slot-89 return (`0x89ABCDEF`),
and four concurrent, distinct slot-89 returns (`0x90000000` through
`0x90000003`). The target also makes two exact fake-backend forwarding calls.
It verifies every event sequence and commit marker after all callback threads
join. This tests the clone/counter/forward trace; it is not proof of an
original engine cold copy.

The harness also runs the source-built target in a hold-after-success mode.
After printing the complete passing trace, that target blocks. The harness
waits two seconds, terminates that synthetic process, drains its pipes, and
verifies it was reaped with a nonzero exit result. This is an abnormal-cleanup
smoke for the synthetic child only.

From the `15bf8ab` renderer checkout root, run the source-built synthetic mode:

```powershell
$py = 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $py -B work/renderer_phase19/host_interception/renderer_contract/fixtures/presentation_probe/build_probe.py `
  --repo-root . `
  --output-dir work/renderer_phase19/host_interception/synthetic_fresh
```

Output must be new or empty beneath checkout `work/`; the path guard checks
lexical and resolved paths and rejects redirected components. The builder
requires the manifest-pinned `msvc5_rtm` files under `C:\tools`, records all
fixture/support source hashes, toolchain/runtime identity, commands and output
hashes, and runs the synthetic target with short waits. The default never runs
the real-system API mode. Do not pass `--run-api` or launch any original image
from this private candidate.

## Original-host boundary still blocked

The existing original-host receipt proves only that the earlier diagnostic
clone was published before the pinned executable's entry instruction. It is
already established; this fixture does not repeat that claim. System
`DirectDrawCreate` create/release is separately observed only in a standalone
source-built diagnostic process. Forwarding from Hercules remains
**UNRESOLVED**.

Static evidence pins the original modules' first table-copy instruction and
destinations: ENG1 `0x2c19f` to RVA `0x70f20`, ENG3 `0x1f84f` to RVA
`0x407a0`, and TITLE `0xc20f` to RVA `0x2bb40`; each uses `rep movsd` for 250
DWORDs before checking the signature. A future debugger can set a hardware
write watchpoint on each destination's slot-89 cell after observing and
hash-validating the corresponding original module's load event. Accept a hit
as a completed copy only when the stopped instruction address is the
post-`rep movsd` address (`0x2c1a1`, `0x1f851`, or `0xc211`) and all 250 words
match the current host clone. This is a prepared method, not a live observation
or a tested debugger implementation.

The post-entry controller is now implemented and exercised against a
source-built host and engine. Its positive and cold/warm paths use actual x86
debug events and exported engine-function hardware breakpoints; the bounded
trace stops after one returned synthetic callback pair. Its invalid-signature
and timeout paths, kill-on-debugger-exit setting, pending-event continuation,
event-handle closure, child termination, and process-exit drainage are
verified in the private receipt. These results do not validate original DLL
events, original-engine copying, or original-host cleanup.

No live run is authorized from this work lane. Before a root-approved run, the
controller must be reviewed to:

1. Pin one exact host variant by SHA-256 (retail
   `587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d` or the
   distinct no-CD copy
   `b3ab3ee9e5c973bb245d8eed795e8e7065d7dfc371e412e602b2059af894a551`) and
   verify the loaded path and file identity. The attach-time PE layout check is
   not a substitute for this distinction.
2. Keep the candidate and host copy in a new, non-redirected ignored `work/`
   directory; retain original ENG1, ENG3 and TITLE files unchanged and reject
   any unexpected path/hash at each engine load event. Record actual module
   paths and hashes; do not replace a game DLL.
3. Resolve the registry-configured installation paths and all save/config
   writes before launch. `docs/eng1_integration.md` documents its separate
   source-built ENG1 experiments but gives no complete registry or writable
   path inventory. The current integration boundary therefore lacks a proven
   disposable user profile and registry view. Copying the EXE, changing the
   working directory, or making only `assets/` read-only is insufficient.
   Preserve the user's registry/profile state and every identified external
   file before any allowed launch, then verify rollback after it.
4. Establish a disposable windowed startup configuration and prove both
   requested/current mode bytes are zero plus the expected restored caption,
   menu and client size. Normal startup requests fullscreen; this candidate
   does not suppress mode changes. Stop if the mode changes or restoration
   cannot be verified.
5. Capture only the first validated original cold copy and one returned
   slot-81 or slot-89 call. The wrapper's completion record is written after
   the saved callback returns; the controller must stop with all threads
   suspended after the wrapper has returned, reject trace overflow/uncommitted
   events, and preserve the original callback return and arguments.
6. Use a short external timeout, kill-on-debugger-exit, explicit pending debug
   event drainage, idempotent handle closure, and a verified abort path. No
   gameplay input, fullscreen route, or independent GPU/presenter experiment
   is part of this gate. Enumerate and review the host's required windowed
   DirectDraw initialization calls before launch; this candidate does not
   suppress or replace them.

Even a successful slot-81/89 run would prove callback dispatch and forwarding,
not a displayed frame, window/palette/movie ordering, display ownership,
shutdown restoration, or native-resolution presentation. Those remain
separate blockers.

## Synthetic post-entry debugger controller

`build_postentry_synthetic.py` extends the candidate with a source-built x86
host and engine DLL. The synthetic host publishes the same 250-word clone,
loads the engine DLL, and invokes its native copy loop on a worker thread. The
controller verifies loaded paths and PE32 image sizes, obtains the engine
state and three marker RVAs from the freshly built export tables, checks that
the markers lie in executable sections, and installs DR0/DR1/DR2 execution
breakpoints on all debugged threads. It uses no code patches.

At the cold marker it compares all 250 copied words with the published clone,
checks slots 81 and 89 point into the candidate proxy, and checks that every
other word still matches the original source table. The positive case then
allows exactly one synthetic slot-81/slot-89 callback pair to return, verifies
its arguments, saved targets, slot-81 return, private physical slot-89 EAX,
and four committed trace events, then terminates the child and drains the
process-exit event. The `coldwarm` case proves a second 250-word copy. The
`invalidsig` case proves table-signature rejection, and `timeout` proves that
the controller kills and reaps a child stopped after the cold copy. The
controller's `--original` mode is compiled as a hard block and returns before
process creation; it reports the original target as `NOTRUN`.

From the renderer checkout root, use a new output directory:

```powershell
$py = 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $py -B work/renderer_phase19/host_interception/renderer_contract/fixtures/presentation_probe/build_postentry_synthetic.py `
  --repo-root . `
  --output-dir work/renderer_phase19/host_interception/postentry_synthetic_fresh
```

The receipt pins the fixture and helper sources, the manifest-verified VC5
RTM toolchain, the Python runtime, each executed image's hash before and after
the test, export RVAs, commands, outputs, and the state-machine results. The
fixture exercises only its source-built host, proxy, and engine. It makes no
OS DirectDraw call and does not launch Hercules or any original DLL. A
synthetic forwarded callback pair is not a displayed frame or proof of
original-engine integration. The original-host gate remains blocked until
profile/registry/write-path isolation, a supported windowed startup, exact
file identities, and separate root review are established.
