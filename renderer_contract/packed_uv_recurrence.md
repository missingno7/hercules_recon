# Phase 7 packed U/V feedback witness

## Result and scope

The sampled texture byte can change a later emitted V byte in the pinned
`D420 -> D60C` row-zero route. In the single selected 319-sample row, texels
fixed to `1` and `255` produce identical UV through `(192, 1)`, then different
V at `(193, 1)`. The runner captures the original x86 registers at the two
operations that cause the difference: `ADD EAX, EBP` and `ADC CH, DH`.

This establishes a finite feedback path for this fixture. It does not establish
that every texel changes later V, nor that every other route uses the same
recurrence. Texture zero transparency only skips the framebuffer store; control
still reaches the V update, although this witness uses nonzero texels.

## Pinned code and setup

Oracle identity: `HERCULES.EXE`, SHA-256
`587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`, image
base `0x400000`. The exact loop setup and body is RVA `0xDC35..0xDCA0`
(exclusive end), size 107, SHA-256
`b8fe7ac9e77137bcd4cb967df60ed0638403f2f8b0853b2bfd572d76b8a48cb4`.
The relevant control spans are D420 `0xD420..0xDD70`, D60C
`0xD60C..0xDD70`, and reciprocal initializer `0x72A0..0x72CC`; exact hashes
and execution preconditions are in [native_packed_uv_feedback.json](evidence/native_packed_uv_feedback.json).

At `0xDC35..0xDC87`, the code initializes the packed lanes as follows:

- `EBX = U_start << 16`; `ECX = U_start >> 16`; then `CH` is replaced by the
  low byte of `V_start >> 16`.
- `EDX = ROL16(U_step)`; then `DH` is replaced by the low byte of
  `V_step >> 16`.
- `ESI = texture_base`; `EDI = destination - 1`.
- `EBP = V_step << 16`, followed by `BP = low16(pixel_count)`. The latter
  changes only EBP's low word, so the V fractional step remains in EBP high16
  while the remaining count occupies BP low16.
- `EAX = V_start << 16`.

For this native row the globals and first-loop values are `V_start=0x00019B4D`,
`V_step=0xFFFFFF32`, `pixel_count=319`, `EAX=0x9B4D0000`,
`EBP=0xFF32013F`, `CH=1`, and `DH=0xFF`. At each iteration `DEC BP` decrements
the count lane, preserving EBP high16; the signed `JG` repeats until BP reaches
zero. This is bounded to the observed 319-sample row and the original screen
width limit.

## Exact recurrence and witness

At `0xDC8A`, the current texel is loaded into `AL`, replacing only EAX's low
byte. The U step (`ADD EBX,EDX; ADC CL,DL`) and optional nonzero framebuffer
store happen before the V step. At `0xDC98..0xDC9A` the V recurrence is:

```text
Eread    = (EAX_before_read & 0xFFFFFF00) | sampled_texel
S        = Eread + EBP                         ; unsigned 32-bit sum
EAX_next = S mod 2^32
CF32     = floor(S / 2^32)
CH_next  = (CH + DH + CF32) mod 256
BP_next  = (BP - 1) mod 65536
```

The `ADC CH,DH` immediately follows `ADD EAX,EBP`, so it consumes that
addition's carry flag. The carry ripples from the sampled AL byte through the
whole 32-bit addition. `CH_next` is the V integer byte used by the following
texture lookup.

The fixture is a strictly convex FT4 with signed-i16 XY
`[(0,0), (320,0), (319,361), (0,361)]`; UV bytes in native vertex order are
`[(0,1), (0,0), (0,219), (0,220)]`. It uses F3 clip bounds `[0,1,320,2]`,
an 8-bit output pitch of 320, and emits x=0..318 on y=1 (319 source reads and
319 stores per run). Native setup yields the horizontal V step
`0xFFFFFF32` (signed Q16.16, or -206 raw Q16 units). The constant textures are
`0x01` and `0xFF`.

At the common pixel `(192,1)`, both variants emit UV `(0,1)`. Before its V
addition, both have `EBP=0xFF32007F`, `CH=0x01`, and `DH=0xFF`. The sampled
byte makes EAX differ:

| Texture byte | EAX after texture read | 32-bit sum with EBP | Carry | CH after ADC | V at next pixel |
| --- | ---: | ---: | ---: | ---: | ---: |
| `0x01` | `0x00CD4101` | `0xFFFF4180` | 0 | `0x00` | 0 |
| `0xFF` | `0x00CDFFFF` | `0x1_0000007E` | 1 | `0x01` | 1 |

The first emitted difference is therefore `(193,1)`: V=0 for texel `1`,
V=1 for texel `255`. The test checks both complete stream lengths before
comparison, the first-divergence location, captured ADD carry, captured ADC
result, native next V, no unexpected memory access, and unchanged original code.

## Reproduction

The public original-code receipt is
[`native_packed_uv_feedback.json`](evidence/native_packed_uv_feedback.json).
It records the pinned executable, exact original spans and hashes, the two
319-sample native streams, the first divergence at `(193,1)`, captured ADD/ADC
witnesses, runtime identity, and bounded scope. The prediction and falsifier
are embedded in the public runner and the receipt; no separate private
`prediction.json` is part of this reproduction.

Reproduce the finite original D60C feedback fixture from the checkout root
with the pinned Python runtime and dependencies:

```powershell
$py = 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $py -B renderer_contract\tests\test_original_packed_uv_feedback.py `
  --exe 'D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE' `
  --original-root . `
  --receipt work\packed_uv\replay_feedback.json `
  --deps 'C:\tools\hercules\renderer-oracle-python' `
  --analysis-deps 'C:\tools\hercules\python'
```

The reusable C state model is [`hercules_packed_uv.h`](hercules_packed_uv.h)
and [`hercules_packed_uv.c`](hercules_packed_uv.c). The original-backed C89
transition replay compiles that model against captured original D60C register
boundaries, including the 319-sample texel pair, a source-zero run, and the
earlier 36-sample packed-U fixture. Its compact receipt is
[`native_packed_uv_state.json`](evidence/native_packed_uv_state.json): this
validates the bounded state transition model, not geometry/span generation,
memory access, another pixel route, or generic rasterization.

Reproduce the original-backed C89 state replay separately:

```powershell
$py = 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
& $py -B renderer_contract\tests\test_original_packed_uv_state.py `
  --package renderer_contract `
  --exe 'D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE' `
  --original-root . `
  --receipt work\packed_uv\replay_state.json `
  --output work\packed_uv\state_build `
  --deps 'C:\tools\hercules\renderer-oracle-python' `
  --analysis-deps 'C:\tools\hercules\python'
```

The runner requires a new or empty build directory and writes generated
objects, executable, logs, and replay receipt only below the specified
ignored `work/` paths. The model consumes fixed-width register values; it
does not dereference target memory or implement a pixel route.
