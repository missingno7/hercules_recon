# TITLE behavioral rendering sources

`src/title/behavioral/render.inc` adds six recovered render functions to the existing `calibration/title/u17cb0.cpp` translation unit. The region index does not include this area; the source provenance comes from the structural draft pointers in `evidence/title_stage1.json`. The explicit behavioral build graph retains this same-unit ownership.

The drafts reuse the original unitâ€™s `TitleObj`, `TitleShape`, `TitlePair8`, `g_2df40`, engine interface, and helpers. I added only the missing `TitlePrim`, `TitleVtx`, `TitleCell`, `TitleVertex`, `TitleFrameBox`, handler type, and external declarations needed for those bodies. The FT4 record has one 40-byte layout. Named unions expose the same UV words as either WORD fields (0x17f50, 0x181e0, 0x186e0) or packed DWORDs (0x18bf0). Its tag is a `TitlePrim *` so the child link and ordering-table links share the primitive type.

| RVA | Signature | Source contract |
|---|---|---|
| `0x17f50` | `void title_17f50(unsigned char *, TitleObj *, unsigned char)` | Decodes a texture-cell stream and emits textured quads with object flip and palette state. |
| `0x181e0` | `void title_181e0(unsigned char *, TitleObj *, unsigned char)` | Emits texture-cell quads with object color, blend, depth offset, and selection handling. |
| `0x184c0` | `int title_184c0(TitleShape *, TitleFrameBox *, int)` | Tests whether a scaled frame box lies outside the 320Ă—240 title viewport; reads `g_2cbe0[2]`. |
| `0x186e0` | `void title_186e0(unsigned char *, TitleObj *, unsigned char)` | Emits texture-cell quads around the object anchor, honoring horizontal flip. |
| `0x18b20` | `void title_18b20(TitleObj *)` | Sets the group transform, invokes the group base handler, and emits each child mesh. |
| `0x18bf0` | `int title_18bf0(TitleObj *, unsigned char *, unsigned long)` | Decodes indexed textured quads, runs handlers and clipping, queues by depth, writes UV state, and returns 1. |

The actual TU names the transform at offset `0x78` `trans_078`; the 0x18b20 draftâ€™s `unknown_078` was rebound to that existing member. The original `TitleObj` declaration also left three consumed bytes/words unnamed. The required private build adaptations name the byte at `0x1e` and WORDs at `0x26` and `0x28` without changing record size. The original 0x18bf0 prototype is `void`, while the draft returns 1, so the private build adapts it to `int`; existing callers discard the result. Those adaptations are listed exactly in `render_receipt.json` for integration.

The fragment compiles under pinned `msvc5_rtm` with `/O2 /Gy` and all six definitions link in the complete source-built DLL. The initial private link experiment had 19 unresolved providers; those dependencies are now supplied by the complete graph. `evidence/title_behavioral_render_data.json` records the three renderer cells, whose initialization and aggregate ownership remain uncertain. The final build has zero unresolved symbols. Renderer equivalence is still unverified; successful linking does not prove displayed pixels, primitive ordering, or resource lifetime.

The only draft-specific code adjustments were to use the existing `TitleObj::trans_078`, read the screen-center cell as `g_2cbe0[2]`, and expose genuine packed-versus-word FT4 storage access through unions. No machine code, binary dispatch, overlapping globals, or recovery state was added.
