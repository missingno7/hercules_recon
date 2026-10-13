# Upstream projection and passive capture

The single-point host path retains camera coordinates that final packets lose.
The public C89 model and a compiled x86 observer have been compared with
unchanged original callbacks in 14 synthetic cases, with 632 assertions.
This is `BOUNDED_EQUIVALENT` for the tested paths, not a live-game hook proof
or historical source match. See [evidence](evidence/upstream_projection.json)
and [cross-module batch paths](cross_module_geometry.md).

## Original single-point contract

All addresses below use the pinned EXE image at base `0x400000`; function
addresses are RVAs and data addresses are absolute x86 addresses.

| Slot / RVA | Physical cdecl contract | Relevant effect |
|---|---|---|
| 101 / B6F0 | One pointer to three WORDs | Copies XYZ; raw EAX combines the argument pointer's high half with the last input WORD, so it is not a clean pointer return. |
| 106 / B920 | No arguments | Calls B740 once and projects; raw EAX is the complete pre-WORD Y sum. |
| 114 / B720 | One pointer to two output WORDs | Copies XY and returns the output pointer bit pattern. |
| 108 / B880 | One source pointer | Copies nine packed signed matrix WORDs, row-major. |
| 174 / B970 | Two DWORD arguments | Sets center X/Y without bounds checks. |
| 175 / B990 | One DWORD argument | Sets focal bits without bounds checks. |

For each B740 row, signed matrix and point WORDs are multiplied; each product
and the three-term sum wrap to 32 bits. The sum is arithmetic-shifted right
12, then a DWORD translation is added with wrap. The result is narrowed to a
WORD and sign-extended back to a camera DWORD. Z below 1 is replaced by 1.
The camera cells are `A8E71C`, `A8E728`, `A8E734`.

B920 wraps `focal << 16` to 32 bits, interprets it as signed, and performs
signed IDIV by camera Z, truncating toward zero. The low 32 bits of each
quotient-times-camera-X/Y product are arithmetic-shifted right 16; wrapped
center addition follows. Only the low WORD of each result is stored at
`A8E740` / `A8E746`. B920/B720 apply no rectangular screen clamp.
Engine clipping and visibility decisions remain separate.

The batch callback B320 is a distinct contract: its statically observed
path does not apply the B740 min-Z clamp, and zero Z skips IDIV. The
cross-module identity fixture verifies its ordered sample lanes, not every
arithmetic discriminator. Do not substitute the single-point model for it.

## Safe observation boundary and ownership

The diagnostic wrapper calls the saved B920 exactly once, copies the three
camera DWORDs and two screen WORDs immediately after return, and preserves
raw EAX, cdecl stack and callee-saved registers. Its native write sequence
matches direct B920 for all 14 cases. It adds no B840 or B810 call.

B840 is not a passive snapshot: it writes camera XYZ into
`A8E770/774/778`, which are the translations used by a subsequent B740.
Adding that call changes later transforms. The still-live result must be
copied before the next point callback overwrites it. The shipped one-record
bridge is diagnostic only; it establishes no production buffer, concurrency
or asynchronous lifetime protocol.

[hercules_projection.h](hercules_projection.h) uses shared fixed-width types
and permits native function-pointer typedefs only with the explicit x86
opt-in. Its config/result structs are joined tool-side views, not invented
contiguous host allocations. Size/offset checks and separate C++ linkage
checks are included. The callback clone helper now accepts passive slots
101, 106 and 114 in addition to its earlier selected slots; publication and
lifetime still belong to the early-table contract.

## Why upstream data matters for native resolution

With identity matrix, translation zero, center `(160,120)` and focal 256,
inputs `(1,0,1000)` and `(2,0,1000)` both become native XY `(160,120)`.
Their retained camera X values are distinct. Scaling packet XY cannot
recover that distinction.

A future private projection can retain it without changing the original
simulation projection. For example, a 1920x1440 4:3 canvas with center
`(960,720)` and effective focal 1536 gives X `961.536` and `963.072`.
These are derived rational policy examples, not original output or a tested
GPU viewport. On a 1920x1080 monitor, preserving the 4:3 view in a
1440x1080 active region with 240-pixel side bars gives X `961.152` and
`962.304`, with Y 540. Source WORD quantization and prior Z clamping remain
precision ceilings even with private reprojection.

Wider visibility is not established: a renderer cannot restore geometry
culled before these callbacks. Exact vertex provenance is also required;
matching equal XY values is insufficient. The bounded
[ENG1 association](eng1_geometry_association.md) identifies additional
producer observers needed beyond the callback table.

## Reproduction

From the renderer worktree root, with the installed pinned Python runtime:

```powershell
python -B renderer_contract/tests/test_original_projection.py --repo-root . --package renderer_contract --exe D:/Prog/hercules_recon/work/discs/pc_install/HERCULES.EXE --output work/replay_projection
```

Outputs are guarded to repository `work/`; originals are read-only inputs.
The runner builds the package and observer using pinned VC5, maps original
instructions in Unicorn, and records original/source/tool hashes and finite
assertions. It does not start a Windows Hercules process.
