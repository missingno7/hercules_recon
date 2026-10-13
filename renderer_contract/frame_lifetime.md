# ENG1, ENG3, and TITLE frame lifetime

This report extends the accepted order-table contract in `renderer_contract/frame.md`. It follows each module’s allocation, order-table reset, packet-arena cursor selection, submission, pointer update, and bounded cleanup path. The preregistered prediction is in the private `prediction.txt`; a separate ENG3 dimensions prediction was not recorded before that follow-up inspection.

The main prediction holds: each module has separate frame/order-table and packet-arena owner cells; reset occurs before producers; submission through the copied `+0x164` callback returns before the outer current-view pointer is updated. The frame-view pointer and packet-arena cursor use distinct, complementary offsets. Restart branches in ENG1 and ENG3 reinitialize their pair before another iteration; their state-1 continuation entries preserve the post-submit pointer rotation. TITLE flips its current display-buffer pointer after each accepted submit path.

HERCULES `AC60` synchronously follows the default order-table chains and returns through each wrapper before the caller changes its current-view pointer. This establishes that the direct `AC60` traversal completes before the swap instruction. It does not prove that a callback or downstream host consumer discards packet addresses when the callback returns. Runtime interface aliases and retained-pointer behavior remain open.

## ENG1

The frame allocation request at ENG1 RVA `0x1F0F0` is `0x2910` bytes through owner cell VA `0x10071EC8`. Setup initializes the two link tables at `*0x10071EC8 + 0x88` and `*0x10071EC8 + 0x1510`, each with count `0x500`. The accepted AC20 writer therefore writes `0x1400` bytes per table. The observed view stride is `0x1488`; the two leading table views account for the request. These are request and write-range facts, not a complete C type or allocator-capacity guarantee.

The packet arena is a separate request through owner cell `0x10070C80`: `80*N` bytes, allocator flags `0x10`, at bootstrap call `0x1F1AC`. In this expression, the index is a signed byte from the live context, selecting the 28-byte row at `0x5DF0E + 28*s8(context[0])`. The code zeroes EAX and loads the row member with `mov ax, word ptr [...]`, so `N` is a zero-extended 16-bit value (the original C declaration is not inferred). The request uses 32-bit x86 `5*N` then `<< 4`; the cursor offset uses 32-bit `40*N`. Since `N` comes from a word, those products do not overflow 32 bits. Adding the offset to the pool pointer is still ordinary 32-bit address arithmetic. Neither the request nor the cursor split proves the arena’s usable capacity.

The current frame-view pointer is the live DWORD at `0x10071EC4`; the allocation base pointer is read from `0x10071EC8`. The per-frame reset at `0x5600` is reached by a direct dispatcher call at `0x2051E`, immediately after `0x2C440`; earlier in that sequence the dispatcher calls `0x2C420`, `0x6005`, `0x2180`, `0x1F870(1)`, and `0x2C440`. There is no `0x50D0` hop on this reset path. The reset passes the **order-table address** `current + 0x88` and count `0x500` to wrapper `0x2C600`. It then stores the packet cursor in `0x10073484`: `pool_base + 40*N` when `current == frame_base`, otherwise `pool_base`.

At `0x20616`, wrapper `0x2C620` receives `current + 0x88` and synchronously dispatches the default slot-89 target AC60. Once that call returns, the dispatcher calls `0x666B` and `0x2CA40(context+0x34)`, toggles `0x10071EC4` between the frame base and base `+0x1488`, increments the context tick, then calls completion gate `0x60F9`. A state-1 dispatcher entry resumes at the iteration and uses the pointer left by the preceding call. A nonzero completion result instead branches through common setup `0x1F320`, which reinitializes both link tables and seeds the current pointer again.

Cleanup at `0x1F220` frees the pointer values in owner cells `0x10071EC8` and `0x10070C80`, then calls bulk release `0x2C2E0(0)`. This establishes the observed release calls, not allocator behavior beyond them.

## ENG3

ENG3 bootstrap function `0x1A330` contains separate allocator calls. Its frame allocation callsite at `0x1A3BC` requests `0x4110` bytes, argument/alignment `0x10`, through frame owner cell VA `0x1004144C`. Setup at `0x1A560` initializes tables at `*0x1004144C + 0x88` and `*0x1004144C + 0x2110`, each with count `0x800`. AC20 writes `0x2000` bytes per table. The view displacement is `0x2088`; the two leading table views span the request. Again, this records request and observed table writes, not a full native aggregate or allocator capacity.

The packet-arena allocation is separate: callsite `0x1A3E4` in bootstrap function `0x1A330` requests `80*N` bytes, argument/alignment `0x10`, through owner cell `0x100405C4`. `N` is loaded as a raw DWORD from `0x10039878 + 32*s8(context[0])` using a 32-bit `mov`; the load does not sign-extend. Its original C signedness is unknown. The request computes `5*N` followed by a 32-bit left shift by four, so the raw DWORD result is `80*N mod 2^32`. The cursor offset computes `5*N*8`, also in 32-bit registers and modulo `2^32`; the pool-base addition is 32-bit address arithmetic. The code shown has no overflow or allocation-result check. These expressions describe request bytes and cursor byte offsets, not guaranteed capacity.

The allocation base pointer is read from `0x1004144C`; the live current-view pointer is in `0x10041448`. Setup initializes both **order-table addresses** at base `+0x88` and base `+0x2110`; on its completion path the other-view cell advances by `0x2088` when it equals the allocation base. Active reset helper `0xA700`, called at `0x1ADC9` before frame callbacks, passes `current + 0x88` and count `0x800` through wrapper `0x1FC10`. It sets arena cursor cell `0x10043164` to `pool_base + 40*N` when `current == frame_base`, otherwise to `pool_base`.

At `0x1AEAF`, wrapper `0x1FC30` receives `current + 0x88` and returns after AC60’s direct traversal. The code then calls `0x0B512` and `0x20030(context+0x34)`, conditionally advances the current-view pointer by `0x2088`, increments the context tick, and calls gate `0x0B059`. State-1 entry branches to `0x1AD7B`, skips the pair setup, and keeps the pointer rotation for the next reset. A nonzero gate result branches to `0x1ACAE`, re-runs setup, and initializes the pair before continuing.

Teardown at `0x1A460` releases the packet arena through owner cell `0x100405C4` and releases the frame pointer read from `0x1004144C`. The inspected span does not clear that frame cell. The behavior of later allocator calls is outside this audit.

## TITLE

TITLE RVA `0x1D380` requests `0x28F0` bytes through frame owner cell VA `0x1002CC08`. Setup at `0x1D590` initializes order tables at `frame_base + 0x70` and `frame_base + 0x14E8`, each with count `0x500`; AC20 writes `0x1400` bytes per table. The observed display-buffer stride is `0x1478`, leaving eight bytes after each measured table interval. These observed bytes do not define all buffer fields.

The packet-arena request is a fixed `0x23000` bytes through owner cell `0x1002BB24`. The primitive cursor is the DWORD cell `0x1002DF40`; the two observed start addresses differ by `0x11800`, and `2*0x11800 == 0x23000`. That arithmetic does not prove a safe producer limit.

The live current display-buffer pointer is in `0x1002CC04`; the frame base pointer is read from `0x1002CC08`. Setup sets current to base, initializes both **order-table addresses** (`base + 0x70` and `base + 0x14E8`), seeds the cursor to `pool_base + 0x11800`, then flips current to the second display buffer. Each accepted `0x05FB0` frame path calls reset helper `0x04660` at `0x0616B` before producers. `0x04660` passes `current + 0x70` and count `0x500` through wrapper `0x0C690`, then selects `pool_base + 0x11800` when current equals base and `pool_base` otherwise.

The frame loop calls `0x0C960(0x80)`, passes **order-table address** `current + 0x70` through submit wrapper `0x0C6B0` at `0x06312`, then flips current by `0x1478` and increments the tick. The inspected cleanup at `0x1D4E0` does not individually release frame owner `0x1002CC08` or packet-arena owner `0x1002BB24`; it calls bulk release wrapper `0x0C340(0)`. That wrapper’s effect on these cells is unresolved.

## Limits and preserved contract

The allocation request, AC20 table-write extent, and packet-arena cursor association are separate claims. The selected arena/cursor is an observed producer storage path; this does not establish that every primitive linked into an order table originates there. GT4 and other per-object or resource-backed packet data may use separate storage. In particular, `40*N` is a cursor byte offset on one selection branch, not a universal arena-capacity bound or a universal `40*N` capacity for all order-table nodes. No arena capacity, maximum producer write, or common ENG1/ENG3/TITLE aggregate is inferred. The modulo-32-bit arithmetic is explicit for ENG3’s raw DWORD count; ENG1’s zero-extended WORD count bounds its size products below `2^32`.

The copied default submit slot is statically bound to AC60. Preserve the existing AC20 count/link behavior, AC60 ascending traversal, next-slot sentinel, NULL termination, primitive prepend order, and host tail callbacks, along with the module-specific reset and post-submit callback order. Runtime callback aliases and any pointer retention beyond direct AC60 traversal remain possible. No presentation fence, GPU ownership, pixel output, or asynchronous-completion claim is made. Existing recovery statuses are unchanged: AC20 is `FUNCTION_MATCH`; AC60 is static-only; the ENG3 and TITLE wrappers retain their recorded `CODEGEN_SIMILAR` statuses.

## Verification

Run from the checkout with an explicit root, four explicit original paths, and a receipt under that checkout’s `work/` tree:

```powershell
& $pythonExe renderer_contract\tests\audit_frame_lifetime.py `
  --repo-root D:\Prog\hercules_recon\work\renderer_interface_repo `
  --exe D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE `
  --eng1 D:\Prog\hercules_recon\assets\pc\HERCULES\ENG1.DLL `
  --eng3 D:\Prog\hercules_recon\assets\pc\HERCULES\ENG3.DLL `
  --title D:\Prog\hercules_recon\assets\pc\HERCULES\TITLE.DLL `
  --receipt work\replay\frame_lifetime.json
```

`--original-root` may be used instead of `--repo-root`; when so used, the checkout root is found from the script location. In either mode all four original image paths remain explicit. The validator pins the four full image hashes and 23 original spans, writes one compact JSON receipt under checkout `work/`, and emits no disassembly or raw byte dump. Outputs overlapping any original-image directory are rejected. The receipt records the hashes of the compatibility evidence inputs. The original binaries are only read.
