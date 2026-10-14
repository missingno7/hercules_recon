# Indexed shade/blend checkpoint

Owned FT4 rectangle commands now carry the selected observed blend table and,
when required, shade row. Capture follows synchronous command consumption;
later texture or aliasing table writes cannot change an earlier snapshot.
Missing table bytes reject nonzero-source commands. Fully transparent commands
need no table snapshot and still preserve the presentation request.

The CPU reference consumes the new shared C8F0/C700 rectangle path. The GPU
uses integer reads from separate R8_UINT source, shade, selected-LUT and prior
destination textures, writing to a distinct integer target. Each call completes
with fence-waited readback; subsequent overlapping calls can use that result as
their destination input. The selected LUT implements arbitrary indexed blending,
without assuming an RGB or alpha-blend formula. Shade-result zero remains a
lookup input, while source zero discards the pixel.

The connected original A710 -> AC60 fixture measures raw red 0x80 and shaded
red 0x7f across all four selectors, two transparent cases and a shade-to-zero
discriminator. Explicit synthetic table writers are inputs to both paths. Their
recipes and hashes are checked before the public runner passes raw bytes through
the modern storage/capture path. Original CPU output is independently checked
against the fixture inputs, then its complete 76,800-index buffer is compared
with modern CPU output and GPU readback.

Modern tests additionally exercise aliasing CLUT writes, immutable earlier table
snapshots, zero-valued lookup results, malformed snapshots, missing tables and
overlapping GPU commands whose second output depends on the first destination.
The original executable has supplied CPU-emulator evidence only in this stretch;
no original game process was launched.

[Measured receipts](blend_slice.json) record the frozen build and regressions.
The shared change and kernel-specific dirty-request evidence are described in
[the contract](../../renderer_contract/indexed_blend.md). The previously measured
copy and geometry slices remain separate checkpoints.

Next boundaries are active CLUT/palette mapping, verified producer commands using
the supported paths, and remaining generic geometry. Original table-writer timing,
movie/GDI composition and exclusive presentation still require evidence. No
post-entry game integration or complete renderer compatibility is claimed.
