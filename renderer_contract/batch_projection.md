# HERC batch projection zero-argument callback fixture

The runner emits a raw report at its fresh `--output` path. The accepted compact evidence is
[evidence/batch_projection.json](evidence/batch_projection.json).
It records two separate proofs: `STATIC_OBSERVED` for the pinned original
spans and `BOUNDED_EQUIVALENT` for a finite original-x86 callback fixture. The runner maps the original
`HERCULES.EXE` and a separately source-built fixed-base x86 PE only in Unicorn,
then executes the three bounded callback spans B430, B320, and B5C0 plus the
fixture PE's zero-argument diagnostic callback.

The zero-argument callback prediction and falsifier are recorded in
[`evidence/batch_projection_zero_argument_prediction.json`](evidence/batch_projection_zero_argument_prediction.json),
with its pre-experiment SHA-256 pinned in the runner. The earlier helper
prediction is preserved in
[`evidence/batch_projection_prediction.json`](evidence/batch_projection_prediction.json)
with its original hash; it describes a multi-argument helper and is not used as
proof of the zero-argument callback. The source includes shared
[`hercules_types.h`](hercules_types.h) and
[`hercules_host_abi.h`](hercules_host_abi.h), uses their fixed-width and cdecl
names, and opts into the header's live-x86 pointer-width guard. The runner
compiles and links an import-free, relocation-free fixture PE, maps its
sections at the pinned base, and records both shared-header hashes.

`STATIC_OBSERVED` pins host table slots 102, 107, and 117 to B430, B320, and
B5C0 and fully decodes the exact spans. B430 is RVA `0xB430`, 106 bytes,
SHA-256 `25367b48038d67ca7a3181586be659fd7a7e15abd9d5a97488eca8fd249c9484`,
ending in RET at `0xB499`. B320 is RVA `0xB320`, 271 bytes, SHA-256
`11368c1fa7fd8cd91b1d7b7de522ad06aacc724991ff4f8d46d7eecf1e7cff97`, ending
in RET at `0xB42E`. B5C0 is RVA `0xB5C0`, 71 bytes, SHA-256
`3bcc224aaa6a9b2fa5772613b2921c7d6341d90d781fdf136f338f3004b382aa`, ending
in RET at `0xB606`.

The bounded B430 invocation reads three cdecl stack values as pointers to
signed-WORD XYZ triplets and deinterleaves them into X WORDs at
`0xA8E708 + 2*i`, Y at `0xA8E70E + 2*i`, and Z at `0xA8E714 + 2*i`. This is an
observed call-boundary layout, not a historical source declaration. B320
stores camera X/Y/Z as signed DWORD lanes starting at `0xA8E71C`, `0xA8E728`,
and `0xA8E734`, with a 4-byte per-vertex stride. It stores projected screen
X/Y as signed WORD lanes at `0xA8E740` and `0xA8E746`, with a 2-byte stride.
For each vertex, it writes screen X then Y. B5C0 copies those pairs to FT4
packet offsets `+0x08/+0x0A`, `+0x10/+0x12`, and `+0x18/+0x1A`. The fixture
records B430's nine scratch WORD stores and B5C0's six packet WORD stores in
order.

The B320 span shows low-32-bit `IMUL` and `ADD` results, three-product row
accumulation with 32-bit wrap, `SAR 12` after the row sum, translation addition
with 32-bit arithmetic, and sign extension of the low WORD into each camera
DWORD lane. These are static instruction observations; the finite fixture
tests sum-before-shift and positive and negative low-WORD narrowing
discriminators. Nonzero Z uses signed IDIV. Zero Z skips IDIV and carries
`focal << 16` into projection. The identity and finite-state tests do not prove
all possible arithmetic order, overflow, or clamp behavior, and no B740 rule
is substituted for B320.

`BOUNDED_EQUIVALENT` compares direct B320 execution with the fixture PE's
genuine zero-argument cdecl diagnostic callback. An exported setup function
stores B320 in persistent C state; the callback reads that saved pointer, calls
B320 once, captures all nine camera DWORD lanes and six screen WORD lanes in
persistent state, and returns the saved EAX bits as `renderer_raw32`. The raw
return is only a physical x86 ABI sample; no source-level B320 type or semantic
return is established. In all three finite cases, direct and callback lane
values, raw EAX, and native B320 write order matched. The callback preserved
EBX, EBP, ESI, EDI, and caller ESP, takes zero explicit stack arguments, and
emits 15 capture stores after B320's lane writes and before B5C0. Matrix and
translation bytes remain unchanged.

The runner checks every executed PC against B430/B320/B5C0 or executable
sections in the source-built fixture PE and enforces per-callback read and
write allowlists. An unknown code or data endpoint fails. A separate
`INT_MIN / -1` case traps at RVA `0xB3F1` in both routes and is reported only
as a non-success trap, not as projection output.

Public replay from the repository root, after the phase-14 shared-header
checkpoint, uses a fresh artifact directory:

```powershell
C:\Users\jiriv\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe renderer_contract\tests\audit_batch_projection.py --repo-root D:\Prog\hercules_recon\work\renderer_interface_repo --exe D:\Prog\hercules_recon\work\discs\pc_install\HERCULES.EXE --output work\replay_batch\batch_projection.json
```

The runner refuses an existing output, a non-empty artifact directory, a
redirected path, output paths overlapping protected reads, and an executable
whose full hash differs from the pinned original. It uses the
existing pinned VC5 toolchain and installed Unicorn/Capstone dependencies.
Center/focal setup, engine-producer execution, other matrix routines, broader
batch domains, and Windows-host execution remain outside this fixture.

The public raw callback typedefs and 48-byte joined batch view are in
[hercules_projection.h](hercules_projection.h). The selected table-clone helper
now also accepts slots 102, 107 and 117; every other transport word remains
unchanged. These declarations preserve physical EAX bits, not an inferred
original C return type.
