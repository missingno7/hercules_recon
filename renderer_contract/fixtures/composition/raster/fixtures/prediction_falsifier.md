# Composition raster prediction and falsifier

## Prediction before the new fixture family

For a bounded synthetic AC60 order-table chain containing one supported
textured quadrilateral packet with small in-range coordinates, unchanged
original AC60 will consume the packet, enter its original polygon/raster path,
write at least one byte in the selected CPU-visible framebuffer, then return
through the original surface tail. A bounded C reconstruction reusing the
existing packet/state/pixel contracts and explicitly controlled memory/service
providers will reproduce the same changed framebuffer bytes and tail operation
order. A packet outside the implemented primitive domain or any missing source,
texture, LUT, surface, or service provider will fail explicitly without
claiming a draw.

## Falsifiers

- AC60 does not route the selected packet to the pinned polygon path.
- The original reaches an unmodeled helper, indirect target, or invalid memory
  access before the raster byte loop or surface tail.
- The native polygon path performs no pixel write for the selected packet or
  writes outside the fixture's bounded CPU surface.
- No supported existing C pixel primitive can be joined to the observed native
  geometry and byte-address result without guessing sampling, clipping,
  ownership, or hidden state.
- The original raster path does not return into the expected unlock/present
  tail, or the C provider order/changed bytes differ from the native run.
- A malformed or out-of-domain packet is accepted as a successful draw.

## Bounds

This is a private bounded unchanged-original x86 fixture using synthetic packet,
texture, LUT, framebuffer, and named service memory only. Unknown code,
memory, or OS/service endpoints are fixture failures. It does not launch the
game, use real graphics services, claim gameplay chronology, infer world/UI
layers, or prove general primitive/frame equivalence.
