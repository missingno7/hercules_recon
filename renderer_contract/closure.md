# HERCULES.EXE AC60 renderer closure

This is a static, bounded closure rooted at original `HERCULES.EXE` RVA `0xAC60`. The oracle identity is SHA-256 `587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`. AC60 is `0x690` bytes, ending at `0xB2F0`; its span SHA-256 is `f7724c04bef582e378b183a12550ba841df1a6fa3660be51da71154aa5c525af`. No original execution was performed.

## ABI and ordering-table walk

AC60 physically reads one 32-bit argument at entry `[ESP+4]`: a pointer to the first 32-bit head cell. The observed ENG1 call path supplies the frameĂ˘â‚¬â„˘s `+0x88` ordering-table view. AC60 saves registers and does not set a defined return value; the host callback path ignores EAX. Original declared type and name remain unproved.

It loads `ESI = *arg` and starts a cursor at `arg+4`. If `ESI == cursor`, ESI is the address of the next head cell. AC60 dereferences that cell and advances the cursor by four, repeating across empty buckets. Otherwise, null exits; a non-null value is dispatched as a primitive. After a primitive is dispatched, the loop follows its first DWORD link. If that link equals the cursor, the primitive just dispatched was the bucket tail; AC60 then dereferences the next bucket cell and advances. Thus a tail primitive is processed before the sentinel is consumed.

The measured frame writer proves a `0x500`-DWORD region at this view. For valid initialized ordering tables, the terminal NULL head/link stops AC60 within the table before a one-past cell read. AC60 has no explicit slot-count bound; behavior for malformed tails or out-of-range/corrupt tables remains unresolved.

## Packet dispatch

AC60 reads the tag at `packet+7`, groups with `tag & 0xE0`, clears bit 1 for base-code comparisons, then tests original bit 1 to choose a raster path. These are minimum consumer accesses in AC60, not complete packet declarations.

| Group and code | AC60 direct access / reachable extent | Direct target |
|---|---:|---|
| `0x20 / 0x24` FT3-compatible | `0x1E` | bit 1 clear: `D420`; set: `E680` |
| `0x20 / 0x2C` FT4-compatible | `0x26` | bit 1 clear: `D420`; set: `E680` |
| `0x20 / 0x3C` GT4-compatible | `0x32` | bit 1 clear: `D420`; set: `E680` |
| `0x60 / 0x64` variable sprite | `0x14` | bit 1 clear: `CAB0`; set: `C8F0` |
| `0x60 / 0x74` fixed 8Ä‚â€”8 sprite | `0x0E` | bit 1 clear: `CAB0`; set: `C8F0` |
| `0x60 / 0x7C` fixed 16Ä‚â€”16 sprite | `0x0E` | bit 1 clear: `CAB0`; set: `C8F0` |
| `0xE0` or `0x00 / 0xE1` page command | `0x08` | inline state update from DWORD `+4` |
| `0xE0` or `0x00 / 0x01` bitmap command | `0x10`-byte direct packet-read envelope | `A8F0`, passed `packet+0x10` and signed WORDs at `+0x0C/+0x0E`; A8F0 reads four signed WORDs at the pointee, for `0x18` total record bytes |
| `0xE0` or `0x00 / 0xF3` clip command | `0x0C` | inline clip clamp/update |
| `0x40` | `0x08` | reads `BA01FC`, then resumes traversal |

Producer-side ENG1/ENG3/TITLE evidence separately supports `0x28` FT4, `0x34` GT4, and `0x14` variable-sprite records. FT3 and fixed-sprite producer extents remain unresolved. Native quirks observed in the existing packet evidence: FT3 duplicates vertex 2 to vertex 3 and has a UV staging quirk; GT4 ignores the other per-vertex color fields; sprite packets use current color/page state without replacing those cells.

## Outputs, gates, and effects to retain

AC60 stages packet-derived coordinates, UVs, color, page selection, and sprite extents in the scratch range `0xA909B0..0xA90A10` (only the individually listed cells in `evidence.json` are claimed). It writes clipped right/bottom outputs at `0x43A488` and `0x43A48C`, clamped to `320Ä‚â€”240` bounds. E1 updates only `A8E6F4 = value & 0x1F`, `A8E6F8 = (value >> 5) & 3`, and `A8E6FC = ((value & 0x180) == 0x180)`; it does not write `A909FC`. AC60 chooses a page base into `A909FC` later on textured primitive paths, using literal bases `0x96E6E0` or `0xA6E6E0`. These globalsĂ˘â‚¬â„˘ original names and ownership are not established here.

F3 reads signed WORD `x` at `packet+4`, unsigned encoded-y byte at `+6`, and signed WORD `dx`/`dy` at `+8`/`+0xA`. It subtracts `0xF0` from encoded-y once if the byte is at least `0xF0`, then independently clamps `x`, normalized `y`, `x+dx`, and normalized `y+dy` to the `320Ä‚â€”240` bounds and writes `A909B0`, `A909B4`, `43A488`, and `43A48C`. The `A5F0` writer stores source DWORDs at `+4/+8`, then writes tag `0xF3` over byte `+7`; thus x occupies `+4/+5` and encoded-y occupies `+6`.

`BA01FC == 0` enables optional setup/lock, polygon and sprite raster calls, and the final color-remap/unlock/present sequence. If `0x96E698` is nonzero on that path, AC60 calls `6EE0` with bytes `0x96E699`, `0x96E69A`, `0x96E69B`, then calls `6FF0`. When `BA01FC` is nonzero, these setup/raster/final presentation calls are skipped; packet traversal, E1 state, bitmap command, clip command, and the final `BD80` gate still execute.

If the gate is zero, the exact tail order is `70C0(0xFF)` (in-place frame color/LUT remap), `7140` (unlock), then `6620` (presentation). AC60 always calls `BD80` last. `BD80` checks callback and gate cells `A8E780`/`A8E788`, then tail-jumps to the registered callback when both are nonzero. It is an essential notification contract even when pixel rendering is bypassed. The outer function itself has no packet counter or explicit count update.

The original has no mutable per-polygon dispatch table: AC60 directly calls `E680`/`D420` and `C8F0`/`CAB0`; the separate slot-131 binding for `A8F0` is not a per-polygon route. There is no indirect call in AC60. Swapping only a raster path would require changing the AC60 implementation or its direct call targets; this binary exposes no narrower renderer callback slot. `A8F0` is a compatibility VRAM rectangle move for command `0x01`, not a disposable rasterizer; the interface source table also binds slot 131 (offset `0x20C`) to VA `0x40A8F0`. Its move and one `BD80` notification are part of the compatibility contract. The other indirect calls in the closure are DirectDraw/service operations inside `6EE0`, `6FF0`, `7140`, and `6620`, plus the final callback jump in `BD80`.

## Closure classification

| Role | Spans |
|---|---|
| Essential external contract | `AC60` packet/frame entry; `BD80` completion callback gate; `A8F0` slot-131 command-1 VRAM move |
| Replaceable rasterizer | `E680`, `D420`, `C8F0`, `CAB0`, and polygon helpers `DD70`, `C700`, `CC50`, `C2E0` |
| Host service to preserve | `6EE0`, `6FF0`, `7140`, `6620`, and lock-failure setter `6EC0` |
| Compatibility side effect | `70C0` frame color remap; `6BB0` lock-failure transition |
| Unresolved | `6850` helper under `6EE0` |

The span sizes, hashes, direct/indirect calls, per-cell data references, packet-relative reads, and input-evidence hashes are in [evidence.json](evidence/closure.json). `6BB0` is a bounded failure-transition entry whose own external-service subtree is not recursively expanded. Callee intervals come from the reviewed function map and were decoded to their RET/NOP ends; this does not prove original COFF/TU boundaries.

Texture-bank range ownership is deliberately unresolved here. AC60 proves page-state cells and literal page-base operands, but not a complete snapshot interval; the concurrent texture audit must establish write bounds before a read-at-submit mirror can claim a bank extent. CLUT and blend-table requirements are likewise outside this receiptĂ˘â‚¬â„˘s measured AC60 span.

## Reproduction

From this worktree, run:

See [README.md](README.md) for the shipped verification commands. Private audit commands are retained only in receipt provenance.

The audit reopens the pinned unpatched HERCULES/ENG1 images, validates image identities and span boundaries/hashes, exact AC60 call targets and indirect-call absence, table-slot binding, direct global references, page-base constants, and SHA-256 identities of the cited frame/host evidence. It uses Capstone 5.0.7. It performs no execution and makes no canonical, recovery, asset, or Git change.

## Integrated reproduction

The private investigation paths above identify provenance. In this package, run tests/validate_originals.py with the original image paths to recheck recorded bytes and bindings, and tests/run_tests.py for the compiled synthetic suite. The byte validator does not independently redo the semantic analysis. The compact receipt is evidence/closure.json.
