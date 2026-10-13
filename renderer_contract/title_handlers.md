# TITLE mode-handler frontier

This static audit closes the four remaining TITLE mode-table entries at
`TITLE.DLL` RVA `0x220d0`. It confirms no new AC60 command family: entries 2 and
3 pass eligible children into the existing mode dispatcher, entry 4 writes
the known `0x68` tile record before optional child rendering, and entry 6
produces the existing FT4 `0x2c/0x2e` records. The inspected original images
were `TITLE.DLL` SHA-256
`b17a37fb1bd5002c54cedb6dba1509bc1ca2d8483fa9724d8e4ade04fe79188b` and
`HERCULES.EXE` SHA-256
`587ae2e90fd2d1827dab6a2745340240e500846f974f718278e2a50199769c1d`.

The eight table entries decode to these image-relative targets:

| Index | Target RVA | Frontier result |
|---:|---:|---|
| 2 | `0x17b90` | Walks an object’s child-pointer list and calls `0x02dc0` for each child. It does not assign one tag; the child’s `+0x23` mode byte selects a table entry. |
| 3 | `0x17bf0` | Sets clip state, filters child objects, then calls `0x02dc0`. Command selection remains child-mode dependent. |
| 4 | `0x17cb0` | Emits a 12-byte `0x68` tile record when span flag `+0x0e & 0x8000` is set, and routes eligible loaded children through `0x02cc0`/`0x02dc0`. |
| 6 | `0x03d10` | Emits FT4-layout records through `0x03100`, `0x03420`, `0x03740`, or `0x039b0`; command high byte is `0x2c` or `0x2e`. |

Entries 2 and 3 first validate/project the parent. On the failed projection
branch they set object byte `+0x1f` bit `0x08` and return. On the successful
branch, entry 2 reads a DWORD count followed by child DWORD pointers at `+0x120`,
then calls `0x04410`, `0x195f0`, and `0x02dc0` for each child. Entry 3 builds a
clip rectangle from parent WORD fields `+0x70`, `+0x72`, `+0x74` and the
projection result; it calls `0x0ca40`, `0x0c7d0`, and `0x0c790` before the
child loop. It passes only non-null children accepted by `0x19470` to
`0x04410` and `0x02dc0`. Thus neither handler is a state-only terminal, but
neither fixes a single command tag either.

The child dispatcher reads mode byte `child+0x23` at RVA `0x2fde` and calls
through the table at `0x220d0` at RVA `0x300d`. The table is therefore a real
producer boundary for entries 2 and 3, rather than an inference from the scalar
arguments passed to another TITLE helper.

Entry 4’s tile path writes color DWORD `0x00ffffff` at primitive `+0x04`, tag
byte `0x68` at `+0x07`, the previous OT head at `+0x00`, replaces that bucket
head with the new primitive, and advances pool cursor `0x2df40` by `0x0c`.
Its consumer status is already covered by
[`evidence/module_packets.json`](evidence/module_packets.json):
AC60 normalizes `0x68` to itself and takes the shared group-`0x60` refresh path,
without entering the `0x7c`, `0x74`, or `0x64` raster branches. Nested children
can still emit the family selected by their own mode.

Entry 6 copies the object WORD at `+0x3e` to `+0x10`, then chooses one of four
writers based on the dimension-derived `0x100` comparison and WORD `+0x74`.
Those writers store the return from `0x189a0` at primitive `+0x04`, insert the
primitive at a depth-selected OT head, advance the pool by `0x28` per record,
and publish the final cursor at `0x2df40`. The original `0x189a0` span clamps
WORD `+0x4a` to `0..255`, constructs the color word with high byte `0x2c`, and
ORs `0x02000000` when flags DWORD `+0x54` has bit `0x04`; this produces exactly
`0x2e` in the high byte for that branch. AC60 reads the command at packet `+7`
and classifies both values under its existing FT4 base code `0x2c`, with bit 1
choosing the established raster path.

No direct call to `0x28c0` appears in the eight table-target spans or the
bounded renderer spans checked by the audit. The known `0x7c`/`0x64` arguments
at other `0x28c0` callsites do not establish packet tags on these paths; this
audit found no link/tag write connecting those values to AC60.

The key original spans and hashes are recorded in
[`evidence/title_handlers.json`](evidence/title_handlers.json), including
the mode table (`0x220d0`, 32 bytes, SHA-256
`415195e248ded9aefc3bba1701c680ed3f995fb163b29400793362439889d23c`), handlers
`0x17b90` (`a2c89c494a260cee9c55c1dec26a7f1bc43be297492f37430ebfe11d347708fc`),
`0x17bf0` (`f38388e55b94044bb6b03c8d1bbab86f771aaaed3e1058da9253ccb3ef5bf666`),
`0x17cb0` (`91152713a992cc3c901de81b8ca2690625b2c56075d0baf4172f591b5d7ee126`),
`0x03d10` (`50ea3d8e83e7cf036aed42effa8c7946ad65c9388cfa673ae08686af449ea7d3`),
the child dispatcher `0x02dc0` (`4b5325e70cf3bac4c0fbc06d0c6c04c9daf4179e70c542338d45ba381be30a08`),
and FT4 word helper `0x189a0` (`fed5051e785d04f377193fd255c278671587fdeabe9ecc2a4dedb7ce41e76b59`).
The receipt also pins the AC60 consumer span and records a size, SHA-256, and
decoded instruction text for each checked instruction site. It includes no
copied original opcode bytes. Every bounded code span is required to decode
completely; the mode table is read as data.

Re-run from the checkout root with Python that can import the installed
Capstone 5.0.7 package:

```powershell
python tests\audit_title_handlers.py `
  --output work\renderer_phase4\title_handlers\audit-output.json
```

The script locates the checkout by `scripts/pe.py` and `renderer_contract`,
then searches the checkout and its ancestors for the pinned originals. If they
are stored elsewhere, pass both `--title PATH` and `--exe PATH`. It reads only
the originals and tracked checkout evidence, and refuses output paths outside
this checkout’s `work/` directory. The committed receipt is
[`evidence/title_handlers.json`](evidence/title_handlers.json). This is
`STATIC_OBSERVED` only; no original execution, gameplay, or new synthetic
fixture was needed because the terminal outputs are already-known AC60 tags.
