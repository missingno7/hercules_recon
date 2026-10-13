# Hercules PC texture and upload evidence

This report is a private, evidence-bounded renderer handoff. It does not change `recovery.json` or claim an allocation size, source identity, renderer replacement, or whole-module match. The original is `HERCULES.EXE`, SHA-256 `587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`, preferred image base `0x400000`.

## Host texture services that must stay coherent

The copied host callback table at RVA `0x361B8` binds slot `+0x204` to A710, `+0x208` to A640, `+0x20c` to A8F0, and `+0x23c` to 70C0. The values are respectively `0x40A710`, `0x40A640`, `0x40A8F0`, and `0x4070C0`. Cross-module evidence confirms A710 is used by all three modules and A640's slot 130 route is TITLE-only. A8F0 and 70C0 are present in the host table, and AC60 also calls both directly; use by every module is not claimed. The table binding is static evidence; runtime table mutation is not excluded.

* **A710 / slot +0x204** takes `(RECT16*, source_bytes*)` cdecl. The descriptor is four signed 16-bit words `{x,y,width,height}`. It copies `2*width` bytes per source row from a tightly packed source (`2*width` source stride), synchronously, and does not retain the source pointer. Each callback calls BD80 exactly once, even with zero height. Its result register is not consumed by the observed caller, so the physical typedef is `void`.
* **A640 / slot +0x208** takes seven cdecl arguments and returns a packed DWORD. The observed first argument is the source pointer. It calls A710 using descriptor low words `{arg4,arg2,arg6,arg7}`; arg3 is not read in this body. A340, which gates its return branches, is the two-instruction `xor eax,eax; ret`; with that return value the packed result is `((((arg4&3)<<2)|(arg5&3))<<3) | (arg2&0x200) | ((((arg4>>2)&0xf0)|(arg2&0x100))>>4)`. The helper preserves this as `herc_tex_a640_return_mode0`. Keep all seven raw arguments and the return behavior together if A640 is mirrored. A640's A710 call also triggers BD80.
* **A8F0 / slot +0x20c** takes `(source_RECT16*, destination_x, destination_y)` cdecl, copies bytes within the same fixed-address map, calls BD80 exactly once, and returns zero in observed native code. AC60 command tag `0x01` directly calls A8F0 with packet `+0x10` as the source descriptor and signed packet words `+0x0c/+0x0e` as destination coordinates; the pointee's extent is not established.
* **BD80** is the notification path reached by both A710 and A8F0. It reads callback cell `0xA8E780` and DWORD gate `0xA8E788`; the gate can suppress the callback jump. Preserve that notification if upload/copy is mirrored.
* **70C0 / slot +0x23c** is separate from the A710 image upload. It consumes the low byte of one DWORD. Non-`0xff` stores a color cell; `0xff` remaps the live `320x240` surface in place using the LUT at `0x91C488`, surface pointer `BA0210`, and stride `BA0214`; color `0x80` bypasses remapping. AC60 also calls 70C0 directly at RVA `0xB2D0` with `0xff`, so slot interception alone misses the present-time remap.

## Addressing, byte meaning, and sampled formats

For the bounded producer band `0 <= y < 512`, `0 <= x < 1024`, a source word's byte `b` (`b` is 0 or 1) lands at

```text
base = 0x96E6E0
row  = y                         when y < 256
     = 0x800 + (y & 0xff)        when 256 <= y < 512
byte_x = 2*x + b
address = base + row*0x100 + ((byte_x & 0x700) << 8) + (byte_x & 0xff)
```

The uploader has a fast contiguous copy when a row remains in one 128-word x-page and a byte-at-a-time path at a page boundary. It copies bytes exactly as supplied. Width therefore counts 16-bit upload words, while a polygon's U coordinate addresses an individual stored byte. The PC path observed here samples one byte per UV coordinate; no 4-bit nibble unpack was observed. This is PC executable evidence, not an assumption imported from PSX texture modes.

The measured inner loops fetch a single indexed byte. Full interpolated UV
wrapping, clipping and edge coverage remain outside the finite pixel proof;
no bilinear or ordinary RGBA texture interpretation is part of this contract.

The equation is also present in the higher-y native branch as `row=0x1000+(y&0x1ff)`, but higher-y writes were excluded from the finite oracle. The `.data` section is not a proved texture allocation. Do not treat either the equation's full index-derived reach or the tested low-y interval as an owned buffer range.

Polygon page selection uses `page=(tpage&0x1f)>>1`, with sample address `base + page*0x10000 + v*256 + u`. When `(tpage&0x180)==0x180`, the selected base is `0xA6E6E0` (one MiB above the ordinary base) and the ordinary page index is replaced. The four blend-table selectors are `(tpage>>5)&3`. These are address expressions; the helper does not dereference them.

The CLUT path converts its word to `row=(clut_word>>6)-0x1e0`; row values outside 0..30 use row zero. For a nonzero row, `[0x96E3E0] != 0` sends D420 through CC50, whose palette lookup is `0x92C390 + row*256 + texel`; its pixel loop skips a zero source texel. For a nonzero row with `[0x96E3E0] == 0`, D420 takes a raw-sample fallback through A90A08 and C2E0, which fills the clipped span including a zero sample. Row zero branches to D60C; it does not enter either described route. Keep this conditional CLUT behavior separate from C882, which uses a per-color byte LUT at `0x91C488 + color*256 + texel`, skips a zero source byte, and blends the old destination through `0x92E3D0 + selector*65536`. The other observed loops CA51 and CBF4 also skip byte zero, using respectively a blend LUT or direct destination copy. These are distinct paths rather than one universal shading rule.

Polygon records carry UV plus high-half TPAGE and CLUT words. Compact `0x74`/`0x7c` records have UV bytes but no CLUT word in their 0x0e-byte prefix, and use the current page state. The `0x64` producer writes a word at `+0x0e`, but AC60 does not read it; its meaning is unresolved and it is not identified here as CLUT. E1 changes current page, blend-table selector, and alternate-bank state. The compact sprite path uses record UV and fixed size (16x16 for `0x7c`, 8x8 for `0x74`); `0x64` carries width/height words.

## Proof and limits

The original A710 and BD80 instructions were executed in the sibling finite Unicorn oracle with synthetic descriptors/source/stack/notification state, and all mapped `.data` writes were checked and restored. Cases covered fast rows `(5,224,7,3)`, a page-crossing slow path `(127,478,3,2)`, and an empty rectangle `(9,240,4,0)`. Observed writes matched the byte address equation; BD80 ran once per case and read its enabled pointer/gate cells. This is finite `BOUNDED_EQUIVALENT` evidence for those A710 cases, not the entire callback domain.

The exact-base native Windows mapping attempt was refused because address `0x400000` was already reserved/mapped with incompatible protection; no native callback was called and no original data was changed by that attempt. No native A640/A8F0/AC60/70C0 execution is claimed here. The private C helper and synthetic tests exercise only the address/format expressions and A640's static mode-zero return expression.

`work/renderer_bounded_oracle/report.json` and `provenance.json` hold the finite fixture proof and tool provenance. `evidence.json` lists the exact original spans, hashes, source evidence, and reproducible commands. The private reference ABI and helpers are `work/renderer_textures/privatehercules_textures.h` and `.c`; synthetic tests are in `candidates/renderer_textures/test_privatehercules_textures.c`. The initial prediction/falsifier is retained in `prediction.json`; its y-row guess was corrected by the dynamic case.

## Integrated reproduction

The [backing and alias audit](texture_ownership.md) establishes a bounded
ordinary 1 MiB address envelope and one fixed alternate 64 KiB byte-UV view.
Neither is an exclusive allocation claim. Extending the alternate view to
sixteen pages would include native page state, reciprocal tables and scratch.
Clipped sprite U/V can exceed byte width, so validate effective addresses
instead of treating one page as a universal sprite read bound.

The private investigation paths above identify provenance. In this package, run tests/validate_originals.py with the original image paths to recheck recorded bytes and bindings, and tests/run_tests.py for the compiled synthetic suite. The byte validator does not independently redo the semantic analysis. The compact receipt is evidence/textures.json.

## Native move order and aliases

Nine finite unchanged-instruction cases now cover A8F0 moves and an A710
borrowed-source alias. Rows advance forward. Within a 128-word page the native
copy uses forward DWORDs then trailing bytes; crossing either page boundary
uses the forward byte fallback. Tested right/down overlaps smear; this is not
memmove, and replacing DWORD chunks with only byte copies can also differ.
Zero height still reaches BD80. The enabled notification fixture stops at its
callback endpoint after writes; it does not execute that callback.
The address helper remains bounded to x<1024, y<512. Negative/out-of-range
rectangles and exclusive allocation ownership remain unresolved.
See [native_vram_move.json](evidence/native_vram_move.json).

Clipped sprite U/V are wider than wire bytes. First-row address is page+U+
(V<<8), without initial wrapping; later row offsets add 256 then mask to
16 bits. The native setup fixture demonstrates U=257,V=258. Packet-u8 sample
helpers therefore must not be reused as clipped-sprite address functions.
See [sprite_geometry.md](sprite_geometry.md) and [palette.md](palette.md).
