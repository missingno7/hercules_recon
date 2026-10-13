# C8F0/CAB0 sprite geometry contract

This receipt covers original `HERCULES.EXE` sprite-entry setup and clipping up
to, but not including, the pixel loops at RVAs `0xCA51` (C8F0 blend route) and
`0xCBF4` (CAB0 copy route). The oracle is the unpatched PC executable with
SHA-256 `587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`.
The recorded prediction and falsifier are in
`evidence/sprite_geometry.json`
with no machine-code change or canonical edit.

Both entries are internal no-argument routines. They read fixed globals,
preserve EBP/EBX/ESI/EDI, and return with a plain `ret`. Neither bounded body
calls another function. AC60 stages signed 16-bit x/y into `A909B8/A909C8`,
zero-extends the UV bytes into `A909D8/A909E8`, and copies variable sprite
dimension words into `A90A0C/A90A10`. Fixed sprite tags stage 8Ă—8 or 16Ă—16
there. Both rasterizers sign-extend those dimension words, so wire values
above `0x7fff` become negative at consumption.

The geometry order is exact and shared by both routes:

1. Return if draw gate `BA01F0` is zero, destination surface `BA0210` is null,
   x is at least 320, or y is at least the dynamic bottom limit `43A48C`.
2. Sign-extend width and height, compute `x+width` and `y+height`, and return if
   either sum is negative. A rectangle wholly left or above therefore exits
   before top/left clipping.
3. If x or y is negative, add that signed coordinate to the corresponding
   16-bit extent, advance U or V by its negation, and set that coordinate to
   zero. U/V updates are 32-bit values with no byte mask. The finite fixture
   starting at U=254,V=253 with x=-3,y=-5 ends at U=257,V=258.
4. If the original right or bottom sum reaches its boundary, shrink only the
   corresponding extent. Right is fixed at 320; bottom is read from `43A48C`.
5. Return if either resulting extent is nonpositive when read as signed 16-bit.
   Any left/top mutations already made remain visible on this empty path, but
   destination/page/blend pointer setup is skipped.

For positive clipped extents, both routes write the destination start
`BA0210 + y*BA0214 + x` to `A90944`, then overwrite `A909FC` with the selected
page base plus adjusted U. They stage the initial source-row offset `V << 8`
at `[EBP-4]`. C8F0 additionally writes
`A90948 = 0x92E3D0 + ((A8E6F8 & 3) << 16)`; CAB0 leaves `A90948` untouched.
The source addresses entering the loops are `A909FC + (V << 8) + column`;
the destination uses `A90944 + row*BA0214 + column`. Static loop instructions
advance the source row by `0x100` and mask the row offset to 16 bits after each
row (C8F0 at `CA7C/CA86`, CAB0 at `CC13/CC1D`). The original first-row V offset is not masked. Pixel and texel behavior is
covered separately by [native_pixel_slices.json](evidence/native_pixel_slices.json).

The Unicorn audit ran 26 finite cases through the original entries and stopped
before the first pixel instruction. It covered ordinary geometry, all four
edges, zero and signed-negative dimensions (including `0x8000`), gate/null
returns, fully out-of-bounds rectangles, and a left-clipped rectangle that
becomes empty. Each result is checked against an independent fixed-width model;
the receipt records the global writes and resulting scratch state. No original
pixel loop instruction executes in this geometry suite.

| Original span | SHA-256 |
|---|---|
| `C8F0..CAB0` | `95ac42d7f4447015eb0760dfccbf2c40fddf600baa76f8f885cf3be7ad9926bf` |
| `CAB0..CC50` | `fe6c00a7e9109250e2e5aa4c3c3419c7381cf9758422d42d319b3e4648d12609` |
| `C8F0..CA51` pre-loop | `e027d326c9c0e30f8c653c4352eb10cc2615b6132407f2bbf9df7b76740a9d35` |
| `CAB0..CBF4` pre-loop | `ba3a45c5845dcd7dfa7b3dcc00ce1c89d7f02279bb4f40eed078593f182eecda` |

The reusable source is [hercules_sprite_geometry.h](hercules_sprite_geometry.h).
The named view is an offline input/output model, not a native memory overlay.
The C fixture checks layout and 12 selected scenarios; it does not compare all
26 native vectors. Its supported bottom-limit domain is 0..32767.
Neither pre-loop body reads F3 left/top/right cells A909B0/A909B4/43A488.
Do not apply one generic F3 rectangle to every primitive family.

Run `tests/test_original_sprite_geometry.py` for the 26 native setup cases
and `tests/run_tests.py` for fresh C fixtures. See [verification.md](verification.md).
Provenance: [sprite_geometry.json](evidence/sprite_geometry.json) and
[native_sprite_geometry.json](evidence/native_sprite_geometry.json).
