# Gate 5: connected composition-order trace

This bounded candidate adds a pointer-free C event record and validates one
connected, bounded original-code sequence. In the `rewrite_link` case,
unchanged AC60 consumes a state-only ordering-table chain, calls the original
command-01 path, synchronously reaches the fixture callback, reloads the
callback-mutated next link at `0xACA2`, and then completes its original
windowed CPU-surface tail through `0x6FF0`, the `0x70C0(0xFF)` remap request,
`0x7140`, and `0x6620`. In this fixture the color guard takes its skip branch,
so the remap loop performs no pixel writes.
The original presenter reaches controlled `StretchDIBits` and `GdiFlush`
endpoints. The run used original `HERCULES.EXE` SHA-256
`587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`.

The frame is already composited in one shared 320×240 indexed image. Packet
shape and coordinate domain can be recorded without asserting semantic
ownership; the fixture contains state commands and no geometry or raster call.

```mermaid
flowchart LR
  subgraph Shared[Shared already-composited 320x240 indexed image]
    Producers[ENG1 / ENG3 / TITLE producers] --> OT[Shared order table]
    OT --> AC[AC60 packet consumption]
    AC --> State[State commands and live link reloads]
    AC --> Raster[Geometry commands / screen-space graphics when present]
    Raster --> View[Locked CPU-visible view]
    AC --> Tail[Optional clear, remap request 70C0, unlock 7140]
    Tail --> View
    View --> Request[Dirty-gated present request 6620]
    Request --> Output[StretchDIBits or DirectDraw Flip]
  end
  Texture[Texture/page writers A640 / A710 / A8F0] --> TexMem[Texture address domain]
  TexMem -. sampling relationship not closed .-> Raster
  Palette[Resource 68C0 / startup 71A0 / entry 6780] --> LUT[Shared palette and LUT domain]
  LUT --> Apply[69A0: unlock, clear, present, then palette apply]
  Apply --> View
  Apply --> Output
  Movie[Movie callback 5450] --> MoviePalette[6780 then 69A0(0)]
  MoviePalette --> MovieCopy[Copy decoded indexed rows to locked CPU view]
  MovieCopy --> View
  MovieCopy --> Request
  MovieRestore[6780 then 69A0(1)] --> Output
  Text[Slot 149 / 6440 TextOutA] --> Desktop[Desktop DC via GetDC(NULL), or attached backbuffer DC]
  Desktop -. visible output and trigger unresolved .-> Output
  Messages[Palette/window/mode and framework paint routes] -. owner/timing unresolved .-> Output
```

The graph separates memory and output domains. The AC60 packet stream is one
shared composition path; primitive kind does not create a world/UI layer.
Texture memory, palette/LUT tables, the locked CPU view, DC/window output, and
final display calls are represented as different event domains. The movie,
palette, text, message, and mode paths are independent routes whose ordering
against a given AC60 frame remains unproved.

## Event representation

`fixtures/composition/src/composition_events.h` and
`fixtures/composition/src/composition_events.c` define the tool-side
API. Each event stores fixed-width integers: scope ID, local order, source
order, source RVA, raw 32-bit target token, four copied arguments, kind,
origin, domain, semantic owner, and flags. Event records hold no pointers. The
collector owns a pointer to its fixed-capacity record array; that pointer is
not serialized. `local_order` is bookkeeping within a scope. Native events use
the original instruction count for `source_order`; C reconstruction events
use their own operation sequence and carry
`COMPOSITION_EVENT_FLAG_C_RECONSTRUCTION`. Separate scopes have no relative
order.

Kinds distinguish geometry commands, screen-space graphics, state commands,
texture mutations, palette/LUT mutations, clear, lock, VRAM move requests,
callback entry/service, live-link reload, packet consumption, remap requests, unlock,
dirty request, present request, movie-frame copy, desktop-DC text, final output,
flush, and unknown writer. Each event stays `UNCLASSIFIED` unless an
independent source association supplies an owner. A capacity overflow,
nonmonotonic order, unknown kind, unresolved writer, or unresolved origin
cannot return a successful complete trace. See
[`fixtures/composition/evidence/event_schema.json`](fixtures/composition/evidence/event_schema.json).

The native event projection and C reconstruction validator check this exact
connected subset, not whole-frame completeness. For the callback-mutated case
the original-code receipt gives this native instruction order:

| Source order | Event | Native evidence |
|---:|---|---|
| 1 | AC60 frame entry | Original instruction RVA `0xAC60` |
| 12 | Windowed surface lock | Original `0x6FF0`; publishes pitch `320` and pixel token `0x34000000` |
| 36 | E1 packet consumed | AC60 boundary `0xACB9` |
| 55, 58, 62 | E1 state stores | Original AC60 stores to `0xA8E6FC`, `0xA8E6F4`, `0xA8E6F8` |
| 69 | Command-01 packet consumed | AC60 boundary `0xACB9` |
| 94 | A8F0 move request | Original A8F0 entry; zero-height rectangle, no VRAM write |
| 110, 117 | BD80 and controlled callback service | Original BD80 followed by synthetic endpoint; fixture rewrites the command's next link |
| 127 | Live-link reload | Original AC60 `0xACA2` reads the rewritten `e1_alt` token |
| 132, 151, 154, 158 | Alternate E1 consumed and applied | Original packet boundary and three subsequent state stores |
| 165 | OT gate consumed | Original AC60 boundary `0xACB9` |
| 187, 201 | Remap request and unlock | Original helpers `0x70C0`, `0x7140`; `0x70C0` skips its pixel loop in this fixture |
| 210 | Present request | Original `0x6620`; the fixture seeded dirty byte `0x43A590=1` |
| 242, 244 | Final output and flush | Controlled synthetic `StretchDIBits`, then `GdiFlush`; no OS/GDI call |
| 249, 256 | Final BD80 and controlled callback service | Original tail call followed by synthetic callback endpoint |

The native receipt pins the full 128-byte original helper span at `0x70C0`
(`3432496f55144a415399893358618b7f970f1a263eb4ea80df30083e6494d4e7`). Its
static branch is `cmp byte ptr [0x436a0c], 0x80` at `0x70D4`, followed by
`je 0x7132` at `0x70DB`. In the fresh `rewrite_link` execution the observed
helper trace is `70C0, 70C4, 70C5, 70C6, 70C8, 70C9, 70D4, 70DB, 7132,
7133, 7134, 7135`; it contains no instruction from the pixel loop beginning
at `0x70DD`, and no instruction writes the guard cell. The native receipt also
contains zero CPU-surface write events. The event is therefore a **remap
request whose remap is skipped in this fixture**, not an executed remap. The
guard value/branch is fixture-specific; it does not establish behavior for
other values of the shared color cell or lock state.

The full instruction counter trace is retained in the fresh native receipt. The
23-event view selects meaningful transitions from that trace; it does not
replace or snapshot the packet chain. The fixture records no clear entry at
`0x6EE0` and no CPU-view byte writes. The state-only case therefore proves
control-flow and provider arguments, not frame pixels or raster output.

## Bounded C reconstruction

`fixtures/composition/src/composition_orchestration.c` independently runs a bounded source model
over the same four raw packet addresses and values. It reuses
`renderer_contract/hercules_frame.c` so the visitor runs before the walker
reads a packet's live next link, and reuses `hercules_staging.c` for E1 state
and command-01 decoding. The synchronous controlled callback provider changes
command-01's next link from `0x10001040` to `0x10001080`; the walker then
consumes the alternate E1 and terminator. Its C event stream independently
emits 23 event records whose ordered kind/domain/RVA/token/argument signatures
match the native trace projection. The native instruction counts and C source
operation counts remain distinct and the comparison excludes both count and
origin fields.

The C test at `fixtures/composition/tests/test_composition_orchestration.c`
explicitly validates the zero-height A8F0 request, the
`6FF0 -> 70C0(0xFF) -> 7140 -> 6620` surface tail, the exact 13 arguments to
the synthetic `StretchDIBits` endpoint, `GdiFlush`, and both synchronous
callback service argument shapes. It does not call an OS graphics service or
write CPU-view bytes. Its `70C0` event records a remap request; the native
trace/static guard audit separately establishes that this fixture's original
loop was skipped. Raster and unknown packets append an unresolved writer
and return `COMPOSITION_TRACE_INCOMPLETE_UNKNOWN`; missing providers are
invalid. The C path is a bounded source reconstruction validated against the
native observation, not a second execution of original code or proof of
pixel equivalence. The audit receipt records hashes for the C orchestration
and reused frame/packet/state sources.

## Routes not joined to this frame

The event schema reserves distinct event kinds for palette/LUT mutation,
movie copy, desktop text, clear, and other screen writers. This frame did not
execute those routes:

- Palette resource callback `0x5760` reaches `0x68C0` then `0x69A0(1)`; the
  palette apply prefix is unlock, clear, present, then mode-specific palette
  update. This ordering is pinned in `renderer_contract/palette.md` and the
  existing screen-ownership evidence, but not in this AC60 run.
- TITLE movie callback slot 148 reaches `0x5450`, writes one movie frame
  directly to the locked view, then unlocks/presents. It bypasses slot 89 and
  has no observed chronology with this frame.
- Slot 149 / `0x6440` uses `GetDC(NULL)`/`TextOutA` in windowed mode or an
  attached-backbuffer DC in fullscreen. Its trigger and visible pixels remain
  unresolved. The probe handoff does not wrap or suppress it.
- Framework paint/message handlers, DirectDraw vtable writers, and mode
  transitions remain separate takeover races. No independent callback
  serialization is established.

The first remaining composition blocker is geometry/raster output inside the
connected live ordering: this fixture deliberately contains no rasterizer
packet and the C path rejects one. Palette and movie order also remain
separate source episodes until an original-backed caller path or passive trace
connects them. A CPU surface writeback or renderer replacement would need the
whole already-composited image and all same-frame host writers; an opaque
320×240 overlay is not supported by this evidence.

## Reproduction

Run the public audit from the worktree root. It starts a new bounded run of the
hash-pinned unchanged original, writes that run's receipt beneath the new audit
output, translates its native trace, builds/tests the pointer-free C API,
builds/runs the independent C orchestration fixture, and compares the ordered
event signatures. The audit does not consume a mutable ignored native receipt.
The output directory must be fresh under `work`.

```powershell
$py = 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
$exe = 'D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE'
$repo = (Get-Location).Path
& $py -B renderer_contract\fixtures\composition\tests\audit_composition.py `
  --repo-root $repo --exe $exe `
  --deps C:\tools\hercules\renderer-oracle-python `
  --analysis-deps C:\tools\hercules\python `
  --cc C:\msys64\mingw64\bin\gcc.exe `
  --output work\renderer_phase19\composition\public_replay_01
```

The native instruction observations come from unchanged original bytes mapped
by the bounded Unicorn fixture. The orchestration and validator are source-
built candidate C tests; they are not the original host's full source or build.
Their status is limited to strict C89 compile/run and the same bounded event
signature. The audit uses the explicit Python 3.12.14 runtime, Unicorn 2.1.4,
Capstone 5.0.7, and GCC 12.2.0. It neither launches the game nor calls real GDI/COM;
the C providers are synthetic. `composition_audit.json` records the newly
generated native receipt hash, static guard decode, compiler commands,
provider run result, C/native sequence comparison, and source hashes. The pinned VC5 bundle additionally compiled and ran the API, stream validator
and orchestration fixtures with `/Za /TC /W3 /MT`. Its independently emitted
23-event signature matched the same native projection. The private receipt
`work/renderer_phase19/composition_vc5_private_02/vc5_composition_receipt.json`
has SHA-256 `acc8e6e14bdcfe3e7ca8c23edf36090710f459e9820eab574a4bd4e938ae2c11`.
One nonfatal C4761 warning concerns the enum-to-byte event-domain conversion;
these fixtures use the recorded domain constants. An earlier attempt used a
separate SP3 installation that failed to start; that is not a limitation of
the pinned bundle used for this check. GCC's strict C89 compile/run also
passed. These compiler results are bounded fixture portability. The audit does
not prove complete visible-writer closure, source identity, pixel output, or
semantic world/UI ownership.

The original-run prediction bytes are preserved verbatim at SHA-256
`3892ff88452c5dd021184c79b85d5a9bbb2544f99951af588374cd0b2969465f`; the
C-model prediction is preserved verbatim at
`761156b5ce9a410a045cbc288f9d2e3a0687f7e2c6c236157f92cfbf594dd896`.
Because the repository's `*.json text=eol=lf` rule can rewrite line endings,
integration must add exact `-text` rules for both package prediction files.
