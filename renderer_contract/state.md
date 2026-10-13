# State-command payload contract

This receipt narrows the AC60 state-command branch behavior to three packet types. It describes observed wire offsets and a proposed header view; it does not claim original C type names, source files, SDK identity, or an accepted renderer implementation. The original image identity and every bounded code span are reproducibly checked by `state_commands_audit.py`; compact hashes and captured references are in `state_commands_evidence.json`.

## E1 page state

AC60 reads the DWORD at packet `+4`. The high byte of that DWORD overlaps tag byte `+7`, so it is `0xE1` after the adapter writes the tag; the state masks below ignore those high bits. For input `v`, AC60 writes exactly:

```text
A8E6FC = ((v & 0x180) == 0x180) ? 1 : 0
A8E6F4 =  v & 0x01f
A8E6F8 = (v >> 5) & 3
```

The E1 branch does not write `A909FC`; AC60 chooses that page base later along textured primitive paths. The adapter source view at `src/pc/host_mode_packet.c` writes DWORDs at `+4` and `+8`, then overwrites tag byte `+7` with E1. AC60 ignores the `+8` DWORD. The exact original span at RVA `0xA540` is pinned in the audit; the source view does not prove which runtime caller produced every E1 packet.

## F3 clip state

The AC60 consumer reads signed WORD `x` at `+4`, unsigned byte `encoded_y` at `+6`, signed WORD `right_delta` at `+8`, and signed WORD `bottom_delta` at `+0xA`. Tag byte `+7` is F3. It computes:

```text
y = encoded_y >= 0xF0 ? encoded_y - 0xF0 : encoded_y  // one subtraction
left   = clamp(x,                 0, 320)
top    = clamp(y,                 0, 240)
right  = clamp(x + right_delta,   0, 320)
bottom = clamp(y + bottom_delta,  0, 240)
```

The right/bottom expressions use the original signed x and normalized y, before clamping left/top. The four results are stored at `A909B0`, `A909B4`, `43A488`, and `43A48C`. The PC helper at RVA `0xA5F0` copies two source DWORDs to `+4/+8`, then writes F3 over byte `+7`: the effective packed bytes are signed x in `+4/+5`, encoded y in `+6`, tag in `+7`, and signed deltas in `+8..+0xB`.

## Command 01 VRAM move payload

AC60 sign-extends WORDs at packet `+0xC` and `+0xE`, then calls `A8F0` cdecl with `(packet+0x10, destination_x, destination_y)`. Its `LEA` at RVA `0xB1E2` forms the pointer and does not read a DWORD at packet `+0x10`; AC60's direct scalar-read envelope is `0x10` bytes. The reachable record includes the A8F0 pointee: A8F0 reads four signed WORDs at offsets `0,2,4,6` from that pointer, so the record extent is `0x18` bytes. The fields are `{source_x, source_y, width, height}`. A610 writes the destination WORDs and copies two source DWORDs to `+0x10/+0x14`, which gives those four little-endian WORD fields. A8F0's VRAM move and its completion notification remain the texture worker's compatibility contract; this receipt only fixes the payload shape and call arguments.

## Header proposal and checks

`hercules_state_wire.h` uses the shared `hercules_types.h` typedefs and packed packet views. E1 reuses the existing `RendererE1ProducerPacket` declaration; this proposal adds only F3 and command-01 views, with unwritten bytes labeled opaque. `hercules_state.h/.c` provides bounded pure decoders: E1 requires its 8-byte AC60 prefix, F3 requires 12 inline bytes, and command 01 requires the `0x18` bytes reachable through A8F0. Each decoder validates its tag, sign-extends the observed WORD fields, stages a local result, and leaves the caller's output unchanged on invalid, truncated, or wrong-tag input. It contains no rasterizer code, native pointer values, or global writes.

`state_wire_layout_fixture.c` checks packet sizes/offsets and synthetic C89 decode vectors with the pinned VC5 compiler. The 43 fixture checks cover E1 masks, F3 y values `240..255` with one subtraction, signed deltas and pre-clamp origin behavior, command-01 embedded rectangle ordering, tag/length rejection, and unchanged output on failure. `test_state_commands.py` independently checks additional synthetic boundary vectors. These are contract checks, not native differential tests. The compiler is Microsoft Visual C++ 5.00.7022 at `C:\tools\hercules\msvc500-8abf95ce980161ad87b0b02402269cce76988953\bin\cl.exe` (SHA-256 `bf9f9c74f756fed96e13f7f9a4273495c7dde0a1fb968e3ef6d760ad6d73dfeb`).

Reproduce the checks from this worktree with:

See [README.md](README.md) for the shipped verification commands. Private audit commands are retained only in receipt provenance.

## Open points

The actual upstream producer/caller semantics for F3 and command 01 remain unproved; A5F0/A610 establish packet writes, not the caller's intended coordinate units. Width/height and coordinate range constraints are not inferred from the four signed WORDs. No behavior for other consumers of E1's `+8` DWORD is claimed. This work adds no pixel rasterizer and does not alter packet decoder files.
