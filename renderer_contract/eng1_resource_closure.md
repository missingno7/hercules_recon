# ENG1 archived-resource point-to-packet closure

This candidate joins an actual archived ENG1 resource-consumer traversal to
the source point and face emitter, then checks the source candidate against
native camera/projection callbacks and emitted FT4 packets. It establishes a
bounded resource-backed geometry route; it does not close the earlier
`2E3B0` preparation/configuration seam.

The native stage read the hash-pinned `MAP_PSX.BIN` member from the original
`HERCULES.FS`, executed original `ENG1+0x20670` and `+0x208F0` resource setup,
then traversed the finite controlled row with `ENG1+0x2DA90`. The selected
resource was mesh table index 156 at map cell offset `0x10D33C`: eight packed
points and three faces. The native trace observed source-point reads at
`2E61B` for the first point and `2E695` for later points; four six-bit indices
from each packed face word select those point records. Pointer and event-order
provenance joins each point read to the corresponding HERC `B6F0` input,
`B740` camera transform, `B920` projection, and `B720` output scratch slot.
This join uses the recorded point index and scratch pointers, not coordinate
equality. The FT4 wrapper at `2C770` copies those indexed XY lanes into packet
corners and links the packet into the observed OT bucket.

The eight-point case emitted the face-index sequence `[5,4,7,6]`, `[3,2,5,4]`,
`[0,1,3,2]`, exercising reordered and reused point IDs. Two other resource
calls provide a culling/winding discriminator: mesh 284 emitted one of three
faces while native `2E3B0` changed depth 384 to 320; mesh 17 entered with
winding 1 and emitted no packets. Across the finite resource traversal the
trace recorded 87 mesh calls, 2,398 packet writes, and 175 OT writes.

The source comparison compiles existing `mesh_emit.c`,
`primitive_emitters_a.c`, and `hercules_projection_eval` with pinned MSVC 5.00
RTM. It derives point inputs and face IDs from the archived map, evaluates
camera XYZ and projected XY independently from each captured B6F0 input/config,
checks those results against the original B740/B920 callbacks, then feeds the
C-derived XY to the emitter. It checks packet bytes, 40-byte packet layout,
culling, and OT insertion for the three cases: 481 assertions pass. The native
250-DWORD host transport is copied first; its source is EXE RVA `0x361B8`,
1,000 bytes, SHA-256
`52654fb050caa552b03c692bd3a1da0e700675bb50605394c8afb92595a039f8`.
Transport word 4 is `0x00BA25C0`, the table base used by the observed native
path.

The executed producer extents remain separately identified: the `2DF20`
GT4 first-return prefix is `0x334` bytes and ends at RET `2E253`. Its FT4
branch at `2E0DA` targets the separately pinned `2E254` tail (`0x156` bytes
through its own RET `2E3A9`). The `2E600` point prefix branches at `2E622` into
the distinct `2E669` tail (`0x4C` bytes through RET `2E6B4`). The traversal,
resource setup, and host callbacks have their own pins in the evidence JSON.
No larger single-function extent or recovery status is inferred from these
tails.

## Evidence and replay

The compact receipt is [eng1_resource_closure.json](evidence/eng1_resource_closure.json).
It binds the original EXE, ENG1 DLL, archive/map slice, full callback table,
candidate sources, toolchain, and fresh report hashes. The detailed point,
packet, write, and emulator traces stay in the generated work output.

The self-contained runner and the three-stage replay commands are in
[fixtures/eng1_real_mesh/README.md](fixtures/eng1_real_mesh/README.md). The
native stage creates a fresh marked output directory under `<repo>/work`;
later stages require that directory and create a fresh compiler subdirectory.
No generated header or replay report is written beside the fixture sources.

## Scope limits

`2E3B0` matrix/translation preparation and live camera setup through HERC
`B880/B8F0/B970/B990` remain native observations supplied to the C comparison
as measured data. The source candidate does not reconstruct those first-party
steps. The row/camera/context are controlled finite inputs rather than live
startup or gameplay state. Three resource cases do not establish whole-map
coverage, dynamic actor identity, packet allocation capacity, or full module
source closure. The result is not a `FUNCTION_MATCH` claim.
