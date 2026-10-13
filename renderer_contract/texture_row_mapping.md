# Latched row mapping for A710 and A8F0

The public helper is integrated into the existing
[`hercules_textures.h`](hercules_textures.h) and
[`hercules_textures.c`](hercules_textures.c); this document describes that API.
It does not introduce a second header, declaration fragment, implementation
file, or texture interface.

`herc_tex_row_mapping_for_entry_y()` models the signed entry comparison against
`0x200`. Select once per callback: A710 uses the destination RECT's signed
WORD `y`; A8F0 uses the source RECT's signed WORD `y`, then uses that same
mapping for both source and destination coordinates on every row.

For the selected mapping and current 32-bit row counter, the address model is:

```text
low-entry row  = (current_y < 256 ? 0 : 0x800) + (current_y & 0xff)
high-entry row = 0x1000 + (current_y & 0x1ff)
byte_x         = 2*x_words + byte_in_word
address        = 0x96e6e0 + row*0x100
                 + ((byte_x & 0x700) << 8) + (byte_x & 0xff)
```

The entry selector and source RECT fields are signed 16-bit values. A8F0's
destination x/y arguments are loaded as full DWORDs and the destination-y
counter increments as a 32-bit register. The address helper uses fixed-width
integer arithmetic, validates only its enum/output/byte selector, and never
dereferences or validates allocation ownership. Its x mask and modulo
arithmetic describe the observed expression; they are not general clipping or
coordinate-wrap guarantees.

## Finite native evidence

The original-code runner executes 13 synthetic A710/A8F0 cases against pinned
HERCULES.EXE instructions. It covers entry-latched rows across 255/256,
511/512, and 1023/1024 transitions; the 511/512 source/destination entry-band
disagreements; horizontal page crossing; A8F0 destination y=32767 to 32768;
and full-DWORD destination x/y witnesses at 65536. The first entry y chooses
the row formula for all iterations; the standalone per-row helper and cached
`+0x100` pointer models each fail at named counterexamples. Each run checks
source reads, destination writes, original text/data restoration, and the
original disabled BD80 gate return. This is finite address/copy evidence, not
producer-domain validity, clipping, memory ownership, overlap semantics, or
whole callback equivalence.

The public C replay is [`tests/test_texture_row_mapping.c`](tests/test_texture_row_mapping.c)
and calls only the existing [`hercules_textures.c`](hercules_textures.c)
implementation. Build it with strict C89 into ignored `work/`:

```powershell
New-Item -ItemType Directory -Force work\texture_row_mapping | Out-Null
& 'C:\msys64\mingw64\bin\gcc.exe' -std=c89 -pedantic -Wall -Wextra -Werror `
  -I renderer_contract `
  renderer_contract\tests\test_texture_row_mapping.c `
  renderer_contract\hercules_textures.c `
  -o work\texture_row_mapping\test_texture_row_mapping.exe
& work\texture_row_mapping\test_texture_row_mapping.exe
```

Run the original-code fixture with the existing repository Python/Unicorn tools.
The runner requires an explicit receipt path under `work/`:

```powershell
& 'C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe' `
  renderer_contract\tests\test_original_texture_row_boundaries.py `
  --original-root . `
  --exe 'D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE' `
  --receipt work\texture_row_mapping\native_texture_row_boundaries.json `
  --deps 'C:\tools\hercules\renderer-oracle-python' `
  --analysis-deps 'C:\tools\hercules\python'
```

The immutable package receipt is
[`native_texture_row_boundaries.json`](evidence/native_texture_row_boundaries.json).
It records the pinned EXE identity, source/runtime hashes, original span hashes,
all thirteen cases, and the observed address streams. A fresh run writes its
own JSON only under `work/`; it does not modify that package receipt.

## Original spans and bounds

Pinned EXE SHA-256:
`587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`.
Original spans: A710 `[0xA710,0xA8E5)`, SHA-256
`30bd84ea8724152e3bfb0e2c521650399913bcec115f1aef489768085981d0fe`; A8F0
`[0xA8F0,0xABF3)`, SHA-256
`bc04260d2227d221b124bb397b3d7f1138eed53081654722e3f1acd52599196e`; BD80
`[0xBD80,0xBD96)`, SHA-256
`6a9578c5be0c980bc35b29642d3ac389e829a0c5666022353cb88c04f19db64f`.
The full evidence receipt pins the discriminating instruction sites and exact
case data.
