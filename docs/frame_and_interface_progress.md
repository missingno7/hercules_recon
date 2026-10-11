# Frame data and ENG3 interface increment, 2026-10-11

TITLE now reproduces the complete **1,492-byte** emitted initialized-data interval
`0x25860..0x25e33`: ten signed-short frame sequences, the wheel/password data,
and the referenced 20-byte `Meg_Speech_Time` trace text. This adds **476 bytes**
to the preceding milestone: 418 bytes of consumed sequence values, 38 bytes of
compiler-emitted alignment, and 20 bytes of text.

The arrays end at their consumed `-1` sentinel. The `999, 776` pairs preserve the
observed sound-marker/argument protocol. No explicit padding arrays, trailing
zero members or alignment controls were added. The named trace char array is an
address-labelled source view of the actual text consumed by `0xd910`; original
literal pooling and declaration identity remain unknown. Its bytes and adjacency
to the password table are reproduced. The native 43-iteration search is unchanged
and still exceeds its C array; defined C behavior and complete runtime equivalence
are not claimed.

All thirteen existing password-unit functions retain their raw code bytes and
masked hashes. Target address bindings and existing `.bss` offsets remain the
same, while compiler-generated `$S`/`$L` names renumber after real declarations
are added. Complete relocation-spelling identity is not claimed. Fresh canonical
linking reproduces all 1,492 data bytes with zero differences. TITLE still has
105 unresolved symbols and no duplicates; the forced layout image is a diagnostic
only, never executed or accepted. Evidence: `evidence/title_frame_data.json`.

The preceding 120-byte first-call/motion interval remains private. Native indexed
operands do not uniquely identify an array base: both a shared `int[30]` and a
first-call scalar followed by a naturally aligned `int[28]`, accessed with
`frame - 2`, reproduce the bytes and effective operand. The split-provider
natural link gives a four-byte scalar-to-motion gap, versus the native eight.
That prediction failed in the current incomplete context; ownership was not
canonicalized and no padding or placement was added. Recover missing upstream
contributions and genuine export/handler context before reopening this family.

ENG3 gains **eleven recovered interface forwarders / 256 contribution bytes**
in `calibration/eng3/interface_wrappers.c`. Entries in `0x1f8e0..0x1fa7f` forward
through slots `0x20`, `0x24`, `0x2c`, `0x30`, `0x34`, `0x48`, `0x5c`, `0x60`,
`0x68`, `0x78` and `0x84`. Fresh VC5 output is relocation-masked equal for every
complete contribution. Each has one DIR32 operand, a matching target relocation
offset, consistent table RVA `0x407a0`, and no referenced interior entry.
PC caller evidence supports argument counts/order; the existing interface header
supplies a compatible physical type view. Original prototypes, signedness and
TU identity remain unknown. No fixture-based or exhaustive behavior claim is
made. Evidence: `evidence/eng3_interface_wrappers.json`.

The new rows remain `CODEGEN_SIMILAR`: actual bytes differ at the resolved slot
operand. Strict acceptance still requires real interface storage and natural
linked addresses. Totals after fresh regression:

| Category | Functions | Contribution bytes including natural padding |
|---|---:|---:|
| Strict `FUNCTION_MATCH` | 113 | 7,840 |
| Masked diagnostic equality | 342 | 81,184 |
| Legacy similar diagnostics | 3 | 352 |
| `RECONSTRUCTED` | 1 | 224 |

All strict instances and all 342 masked rows in 30 sources pass fresh compilation;
the 45 Python tests pass. `recovery.json` remains the only recovery authority.
No strict byte gain or whole-module equality is claimed.

Next complete ENG3's actual interface storage/export dependency or another
evidenced wrapper region while TITLE's ownership question is blocked. For TITLE,
measure a genuine missing handler in the recovered frame context. Retain failed
register families until changed context supplies a concrete new discriminator.
