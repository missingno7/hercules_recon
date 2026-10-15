# Host-interface shim prototype (experimental)

This is a compiled x86 DLL that interposes on selected slots of the
250-DWORD host→engine table. It forwards every call to the original
unchanged and gives a backend-neutral render-command interface to a future
native renderer.

It is modern-layer code, not reconstruction source. It never runs Hercules.
Everything is proven against a synthetic host and against the original AC60
bytes under Unicorn.

```powershell
python prototypes/host_shim/build.py --exe work\discs\pc_install\HERCULES.EXE --output work\host_shim_run
```

This builds `hercules_shim.dll` (PE32 i386, VS2022 `cl` 19.44 x86, `/W3 /WX`)
from `src/host_shim.c` plus the shared `renderer_contract` frame, packet,
staging, state and clone sources. It then runs both test layers. The receipt
is [evidence/build_test_receipt.json](evidence/build_test_receipt.json).

## What the DLL does

- **Publication.** `hs_install` validates the source table, builds a process-lifetime clone through `engine_table_clone_selected`, and publishes it with compare-exchange. It never runs from `DllMain`, and there is no live attach.
- **Relay.** Thirteen allowlisted slots are relayed: 81, 89, 101, 102, 106, 107, 114, 117, 129, 130, 131, 143 and 148. Each points at a naked stub, `mov eax, slot; jmp hs_relay`.
  - The relay copies an 8-DWORD stack window and passes the caller's ECX/EDX through.
  - It returns the original's EAX/EDX untouched and preserves EBX/ESI/EDI/EBP. Callers still clean up (cdecl).
  - Arity is the callee's business, so unknown signatures need no wrappers.
- **Frame model.**
  - Slot 81 records each order table's extent `(first, count)`.
  - Slot 89 pre-walks the order table with `frame_walk_ac60_order_table_visit` *before* forwarding. It decodes each node (`renderer_decode_packet`) and applies it to a shim-owned `RendererAc60StateView`, then emits `frame_begin` / `primitive` / `frame_end`.
  - The pre-walk runs only when the primary BD80 callback cell is NULL and the extent is known. Otherwise AC60 could run foreign code that changes the consumption order.
  - After the original returns, a re-walk must reproduce the pre-walk exactly. That is a built-in falsifier, reported as `consumption_consistent`.
- **Fault firewall.** Backend calls run under `__try/__except` with x87 control word and MXCSR save/restore. A backend fault disables the backend and the game continues. Original-code exceptions are never caught: a `__finally` only closes an abandoned slot-89 bracket while the host unwinds.
- **Backend switching.** `hs_select_backend` swaps the backend or mode between frames. The published table never changes, so gameplay-visible calls are identical with any backend or none, which is tested.

## Verification

**Synthetic host: 346/346 checks.** The fakes are cdecl services, including
an AC20 with the original's shape and an AC60 loop transcribed from the
decompile. The "engine" copies the table as `rep movsd` would and calls
through an assembly caller that checks ESP balance and callee-saved registers.

- Every relayed slot matches a direct call to the same service in EAX, forwarded arguments (up to 7), LastError, and inbound ECX/EDX. EDX is compared where the service defines it.
- Slot 89 returns EDX:EAX intact. The pre-walk order equals the consumed order, and the event bracket order is S‑B‑P…‑E‑S.
- Forward-only, observe, and faulting-backend runs produce **identical host call logs**.
- An access violation or C++ throw (`0xE06D7363`) in the backend is contained, and the backend is disabled. A backend that changes x87 precision has the control word restored.
- Original exceptions (slot 143 and slot 89) propagate unchanged, and the abandoned bracket recovers.
- The pre-walk is skipped for a non-null primary callback or an unknown extent. A consumed link rewritten by the original is detected.

**Original-code differential, `tests/ac60_oracle.py`: 9/9.** The harness's
order-table image is mapped at its real addresses and the pinned EXE's AC60 is
executed with all 11 callees stubbed:

- The original consumption order equals the shim pre-walk (10 primitives over 15 buckets).
- AC60 performs **no writes** to the order table or packets. That is the precondition behind "pre-walk == consumption".
- The final E1 page cells (`A8E6F4/F8/FC`) and F3 clip cells (`A909B0/B4`, `43A488/8C`) equal the shim's staged model.
- The command-01 `A8F0` arguments equal `node+0x10` plus the staged destination.
- Raster routes match: opaque FT4/GT4 go to `D420`, semi-transparent FT4 to `E680`, and sprites 0x64/0x74/0x7C to `CAB0`. Tag 0x68, E1 and F3 draw nothing.
- Frame prologue is `6FF0`. The epilogue is `70C0(0xFF)`, `7140`, `6620`, `BD80`.

The differential caught a real modeling error. The original AC20 writes
`count-1` links and the NULL in cell `count-1`, which the first fake got wrong.

## Architecture assessment

New constraint from this work: the engine frame runs **inside MFC `OnIdle` on
the UI thread**. The `CWinThread::Run` idle loop calls `0x79A0`, which ticks
the engine (export, then AC60 raster and present) under an `__except` that
turns any fault into a fatal `exit(-123)`
([callback_metadata.md](../../renderer_contract/callback_metadata.md)).

| Mechanism | Sees | Misses | Role |
| --- | --- | --- | --- |
| **A. App-local `ddraw.dll` proxy** (existing `dropin_renderer/proxy`) | Loads before host code (static import, attach-time), DirectDraw objects, fullscreen surfaces, Flip/Lock, palettes | Geometry and packets; windowed presentation (GDI `StretchDIBits`) | Bootstrap vehicle; presentation ownership |
| **B. Host-interface shim** (this prototype) | Packets in consumption order, staged state, uploads (129/130), VRAM moves (131 and cmd‑01), remap 143, movie 148, raw upstream projection inputs (101–117) | Host-internal edges: AC60→`70C0`/`6620`, movie lock/present, text `6440`, palette `68C0`/`69A0`, clear `6EE0`, mode `6BB0`/`6A60`, WM_PAINT | Render-command source |
| **C. Engine-DLL proxies** (ENG1/ENG3/TITLE) | The same table as B, at `PC_DLLEngineMain` | Same as B. Needs three proxies and renamed originals in a staging copy | Not preferred |
| **D. Reconstructed engine source** | Everything upstream | Gated on matching progress | Long-term path to true 3D |

**Recommended smallest reliable design: A hosting B in one app-local
`ddraw.dll`.**
- Attach-time publication of the B clone. This is static data only, and the pre-entry probe already proved this boundary on the real EXE.
- Lazy forwarding of `DirectDrawCreate` (existing proxy code).
- The `HsBackendV1` interface for renderers.

B alone cannot be bootstrapped without A. A alone has no render commands.

Constraints a backend must respect, all derived from evidence:
- **Threading and timing.** Every event is synchronous on the UI thread inside OnIdle. A backend that blocks, for example on vsync, throttles the idle loop. Whether that changes simulation speed depends on `5F20`'s timing policy, which is not yet characterized: measure before presenting from a GPU backend. Never pump messages from a backend.
- **Faults and FPU.** The firewall is mandatory. Without it, a driver fault becomes the host's fatal exit. D3D-style x87 precision changes would leak into game code.
- **Lifetimes.** The clone and the relay must live for the whole process, because engines copy once (cold copy); a static import is never unloaded. Order-table nodes and packets are engine memory, valid only inside the slot-89 bracket, since the order tables are double-buffered. Copy anything you keep.
- **Texture memory.** Host texture memory (`0x96E6E0`/`0xA6E6E0`) is mutated synchronously by slots 129/130/131 and command‑01 nodes, all observable here. Exclusive ownership of those banks is still unproved (contract open question 8).
- **Palette.** Slot 145 (palette resource) is outside the contract's clone allowlist. Adding it needs evidence, so palette tracking currently relies on host cells.

### Path to a native renderer (not implemented)

A "replace" mode for slot 89 must still perform AC60's non-pixel effects:
- the E1 page cells;
- the F3 clip cells (`A909B0/B4`, `43A488/8C`);
- command‑01 through the **original** `A8F0` (VRAM contents later frames sample);
- the `BD80` notification and the `BA01FC` gate semantics.

It may drop `6FF0`/raster/`70C0`/`7140`/`6620` only if A simultaneously owns
every other writer of the visible frame (movie, text, palette apply, clear,
mode transitions, paint). That is the existing presentation-takeover blocker,
and it is unchanged.

Packets are integer 320×240 screen space with no Z. Native-resolution
precision therefore needs the upstream projection inputs this relay already
forwards as raw events (101–117), plus per-module association. ENG1 needs
observers before face selection (`2E0F1`/`2E27B`). Association is the gating
problem for "true" native geometry, not interception.

### Not done (gated)

No live attach, no original post-entry execution, and no combination with the
real `ddraw.dll` proxy. The AGENTS.md gate (explicit authorization, isolated
writable state, windowed startup, display restoration) still applies.
