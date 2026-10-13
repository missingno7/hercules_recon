# PC renderer packet proposal

This is a private wire-view proposal for the original PC renderer. The names
FT3, FT4, GT4 and sprite are shape labels for observed PC command families; no
PSX aggregate or ABI is assumed. The original consumer evidence is the static
AC60 trace in `work/renderer_closure/evidence.json` and the unpatched
`HERCULES.EXE` span RVA `0xAC60..0xB2F0` (SHA-256
`f7724c04bef582e378b183a12550ba841df1a6fa3660be51da71154aa5c525af`).

Prediction and falsifier: the tag at byte `+7` selects fixed PC layouts whose
link is a raw DWORD at `+0`; polygon fields repeat at eight-byte strides for
FT4 and twelve-byte strides for GT4, while sprite layouts carry their own
dimensions or use fixed command dimensions. The prediction is falsified by an
AC60 load/store outside the offsets below or a native producer that uses a
contradictory record extent/field order. The data supports the prediction for
FT4, GT4 and variable sprites; FT3/fixed sprites still lack producer extent
evidence.

## Consumer command families

AC60 reads the packet tag at `+7`, compares the base code after clearing bit 1
(`tag & 0xfd`), and separately uses bit 1 to choose one of two rasterizer
paths. Thus both opaque/semi variants are recognized. Bit 1 is recorded as a
path selector; a modern blend equation is not inferred.

| Base tag | Observed form | Direct AC60 minimum | Known allocation evidence |
|---|---|---:|---|
| `0x24` | Three XY/UV vertices; AC60 duplicates vertex 2 into output vertex 3 | `0x1e` | Unresolved |
| `0x2c` | Four XY/UV vertices, one packed color/command word | `0x26` | ENG1 draw-command and TITLE writers use `0x28` bytes |
| `0x3c` | Four XY/UV vertices; ENG1 producer also writes one RGB triplet per vertex | `0x32` | Native ENG1 mesh producer `0x2df20` reaches the GT4 helper and advances by `0x34` bytes |
| `0x64` | Variable-size sprite | `0x14` | ENG1 tile, ENG1 command writer and original ENG3 writer use/advance `0x14` bytes |
| `0x74` | Fixed 8x8 sprite | `0x0e` | Unresolved |
| `0x7c` | Fixed 16x16 sprite | `0x0e` | Unresolved |
| `0xe1` | Texture/page state command; reads DWORD `+4` | `0x08` | ENG1 command candidate allocates a 12-byte extra record through helper `0x2c980`; this is writer evidence, not AC60's access extent |
| `0x01` | Rectangle copy/move route to `0xa8f0`; address `packet+0x10`, signed words `+0x0c/+0x0e` | `0x10` direct bytes | A8F0 reads four signed words through the pointer (`+0x10..+0x17` if inline), so transitive bytes reach `0x18`; producer allocation remains unresolved |
| `0xf3` | Known clip-rectangle state update | `0x0c` | Unresolved |

These minima describe bytes directly accessed by AC60; they do not establish
complete C object sizes. The decoder preserves unsupported `0x01` and `0xf3`
as known-but-unsupported commands and returns `TRUNCATED` until their direct
access minima are present. Other tags return an explicit unknown status. The
`0x40` group is an observed traversal gate, not a decoded packet family.

## Layout fields

All offsets below are byte offsets from the record start. `link` is a raw
32-bit address/sentinel value. The decoder never converts it to, or
dereferences, a native pointer.

| Field | FT3 | FT4 | GT4 | Sprite `0x64` | Fixed sprite `0x74`/`0x7c` |
|---|---:|---:|---:|---:|---:|
| Link | `+0x00` | `+0x00` | `+0x00` | `+0x00` | `+0x00` |
| RGB / command | consumed RGB + command `+0x04..07` | consumed RGB + command `+0x04..07` | RGB0/cmd `+0x04..07` | producer bytes `+0x04..06` ignored; command `+0x07` | producer bytes `+0x04..06` ignored; command `+0x07` |
| XY0 | `+0x08` | `+0x08` | `+0x08` | `+0x08` | `+0x08` |
| UV0 / CLUT | `+0x0c / +0x0e` | `+0x0c / +0x0e` | `+0x0c / +0x0e` | UV `+0x0c`; producer word `+0x0e` is not consumed | UV `+0x0c` |
| RGB1 / XY1 / UV1 / TPAGE | Ă˘â‚¬â€ť / `+0x10` / `+0x14` / `+0x16` | Ă˘â‚¬â€ť / `+0x10` / `+0x14` / `+0x16` | producer RGB1 `+0x10` ignored / `+0x14` / `+0x18` / `+0x1a` | dimensions at `+0x10/+0x12` | dimensions implied by tag |
| RGB2 / XY2 / UV2 | Ă˘â‚¬â€ť / `+0x18` / `+0x1c` | Ă˘â‚¬â€ť / `+0x18` / `+0x1c` | `+0x1c` / `+0x20` / `+0x24` | Ă˘â‚¬â€ť | Ă˘â‚¬â€ť |
| RGB3 / XY3 / UV3 | Ă˘â‚¬â€ť | Ă˘â‚¬â€ť / `+0x20` / `+0x24` | `+0x28` / `+0x2c` / `+0x30` | Ă˘â‚¬â€ť | Ă˘â‚¬â€ť |
| Direct minimum | `0x1e` | `0x26` | `0x32` | `0x14` | `0x0e` |

XY words are interpreted as signed 16-bit coordinates, matching AC60's
sign-extended staging. UV values are bytes. In polygon packets, CLUT and TPAGE
occupy the upper half of the relevant UV words at the noted offsets. Variable
sprites carry width and height as unsigned words. Original ENG3 writes a word
at sprite `+0x0e`, but the AC60 `0x64` branch does not read it; it is therefore
reported as a producer-populated, consumer-ignored word, not as a consumed
CLUT. AC60 also does not consume per-record RGB bytes for compact sprites; the
observed sampler path uses current renderer color/page state. Fixed commands
imply square dimensions; AC60 does not read dimensions from their record.
Polygon packets take texture/page selection from their UV words, while sprites
use the current page state set by an `0xe1` command.

GT4 RGB triplets at `+0x10`, `+0x1c`, and `+0x28` are observed in the original
ENG1 mesh producer layout. AC60's GT4 path ignores those later color triplets;
the decoder exposes only color lane 0. Producer-derived shade/color behavior
is separate from the packet's access minimum. The source-only `mesh_emit.c`
candidate models an FT4 branch too, but the native `0x2df20` body directly
reaches `0x2c790` only; this packet note does not claim it as an original FT4
producer.

No independent depth member appears in these records. ENG1 chooses an OT
bucket from its depth value before linking the packet; depth is therefore
outside this wire record. A coarse depth-derived shade contribution can also
be folded into UV/texture data upstream. No depth reconstruction is claimed.

## Ordering-table safety boundary

AC60 receives one pointer to four-byte head cells. Its current slot's next
bucket address is used as a sentinel: a head equal to that address is a bucket
cell, so AC60 advances without reading `+7`; otherwise the head is processed
as a packet and its first DWORD is followed. This is how the native loop
distinguishes a four-byte bucket from a primitive. The original loop has no
slot-count check and advances by loading the next head cell. A proposed safe
adapter must supply a module-specific slot count and bounded address resolver,
compare links to a computed sentinel, and stop the final bucket without
dereferencing a one-past-table cell. The packet decoder alone accepts an
already bounded byte span and requires at least eight bytes before reading
command `+7`.

Frame envelopes remain module-specific. Use the independently recovered ENG1
and TITLE descriptors for table base/slot count; this proposal exports a
`RendererRawOrderTable` descriptor and checked raw bucket/sentinel address
calculation, plus fixed-width packet records and command decoding. It does not
export a shared full-frame struct. The final sentinel is only an address value;
the caller must stop there rather than read it. Parent integration owns the
common width header.

## Proof boundaries

- `STATIC_OBSERVED`: AC60 access offsets/dispatch, the original ENG3 sprite
  writer instruction window, and source stores in the ENG1 mesh candidate.
- `LAYOUT_VERIFIED`: the proposed packed structs compile under pinned VC5 with
  `sizeof` and `offsetof` assertions. This verifies C declaration geometry,
  not historical source identity.
- `BOUNDED_EQUIVALENT`: existing ENG1 tile and draw-command differentials, and
  the listed TITLE FT4 producer differentials, within their recorded finite
  buffers and controlled helper limits.
- `UNRESOLVED`: FT3 and fixed-sprite producer sizes; complete original packet
  aggregates; command `0x01` pointee layout; source identities; rasterizer
  pixels; live packet/table extents.

The candidate decoder is checked against synthetic byte fixtures only. No
bounded-equivalence claim is made for AC60 rasterization or output pixels.
Rebuild the original-span/source hashes with
`python work/renderer_packets/audit_renderer_packets.py`; compile and run the
C89 candidate fixtures with `powershell -File work/renderer_packets/run_tests.ps1`.

## Integrated reproduction

The private investigation paths above identify provenance. In this package, run tests/validate_originals.py with the original image paths to recheck recorded bytes and bindings, and tests/run_tests.py for the compiled synthetic suite. The byte validator does not independently redo the semantic analysis. The compact receipt is evidence/packets.json.
