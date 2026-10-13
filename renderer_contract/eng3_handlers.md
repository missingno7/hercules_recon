# ENG3 handler-table frontier: indices 0–3

**Prediction recorded before tracing:** these entries would be state/delegation paths and/or producers of already-known AC60 families; no new packet family was assumed. The falsifier was a hash-checked direct route writing a command byte outside the documented AC60 families or a newly supported layout. The trace found no such counterexample.

The table at `ENG3.DLL+0x35010` is the same six-entry table already pinned in `renderer_contract/evidence/module_packets.json` (SHA-256 `22545ddc…f742e73`). Entries 0–3 resolve to `0x9030`, `0x90e0`, `0x22de0`, and `0x22e40`; entries 4–5 remain covered by the existing public receipt and were not re-traced here.

| Index | Entry | Observed route | Packet frontier |
|---:|---:|---|---|
| 0 | `0x9030` | Data-dependent calls to `0x9420`, `0x90e0`, or `0x99b0`. | All three are FT4-compatible linked writers using helper `0x23b00`; helper evidence already documents command tags `0x2c`/`0x2e`. Each advances the packet cursor by `0x28` and links through the order-table head. |
| 1 | `0x90e0` | Direct writer. | Same FT4-compatible wire fields and `0x28` cursor step as index 0. |
| 2 | `0x22de0` | Conditional state/object helper route. One branch calls `0xa410`, then shared renderer `0x8da0`. | `0xa410` synchronizes object fields `+0x40/+0x42/+0x44` from `+0x34/+0x36/+0x22`; no packet layout is written at this entry. Rendering is delegated. |
| 3 | `0x22e40` | Geometry/state preparation, then shared renderer `0x8da0`. It also calls host wrappers for slots 103, 108, 121, and 164 on the traced path. | No packet layout is written at this entry. Wrapper targets are recorded as host callback boundaries; their semantic bodies are outside this packet audit. |
| 4 | `0x22f00` | Previously accepted linked tag-`0x68` writer. | Existing public evidence says AC60 refreshes shared page state and skips raster for `0x68`; not re-traced here. |
| 5 | `0x23440` | Previously accepted direct FT4 writer. | Existing public evidence covers tags `0x2c`/`0x2e` and `0x28` cursor stride; not re-traced here. |

The three direct writers write packet-relative offsets `{0,4,8,0xc,0xe,0x10,0x14,0x16,0x18,0x1c,0x20,0x24}` through the packet pointer, with a 4-byte order-head update through the head pointer. The highest observed packet write ends at byte `0x25`; the cursor increment is `0x28`. That increment is a producer stride, not proof of backing-buffer capacity. The family reuses the public FT4 packet declaration; this trace adds no wire type.

The shared object renderer `0x8da0` contains a direct route to the existing FT4 writer `0x231a0` at `0x901d`. It reads object byte `+0x23` at `0x8fbe`; zero calls handler 0 at `0x8fd0`, while the nonzero path indexes the same table at `0x8fed`. Thus indices 2 and 3 can reach known FT4 emission through a shared renderer, but the selected nonzero child handler remains data-dependent. This receipt does not infer that every child emits a packet.

The `0xa410` sync sites record the byte read at `+0x22`, source WORD reads at `+0x34/+0x36`, and conditional WORD writes at `+0x40/+0x42/+0x44`. The traced callback cells resolve through the original host table to RVAs `0xb840` (slot 103), `0xb880` (108), `0xb870` (121), and `0xbb00` (164); their names and gameplay meanings are not inferred here.

All claims are `STATIC_OBSERVED`. No bounded equivalence or runtime execution was performed, and no absence claim is made for other ENG3 paths. Existing indices 4–5, AC60 dispatch minima, and the FT4 field semantics are reused from `renderer_contract/evidence/module_packets.json` and `renderer_contract/evidence/packets.json`.

Rerun from the repository root with the pinned Codex Python runtime and the public `scripts/env.py` dependency setup:

```powershell
& 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' renderer_contract\tests\audit_eng3_handlers.py
```

The audit reads the original ENG3 DLL and HERCULES.EXE named by the public geometry evidence, verifies their pinned hashes and the indexed handler table, then writes only ignored `work/renderer_bounded_oracle/eng3_handlers.json`.
