# REA-assisted camera dependency recovery

ENG1 `0x20ea0` now has historical C source in
`src/eng1/behavioral/camera_bounds_20ea0.c`. It removes one genuine camera
dependency: at adoption the source graph dropped from 531 to 530 unresolved symbols, with 368
mapped entries, 399 function symbols and no duplicate providers. It still
produces no DLL or gameplay frame.
The subsequent [camera-chain recovery](eng1_camera_chain.md) corrects camera step
selection and supplies the non-level-6 part of `26730`, reaching 529 unresolved.

REA 6.5.0/Ghidra 12.1 supplied a useful structural view of packed resource rows.
It incorrectly inferred six parameters, fastcall and a 64-bit return. The caller
and instructions establish four cdecl stack arguments: output pointer, signed
row index, mode and position. EAX reports success; EDX is incidental.
[Compact evidence](../evidence/eng1_camera_bounds_20ea0.json) pins the original,
seed/query transcripts, compiler inputs and tests. REA is not a build dependency.

The helper reuses `g_root_count_70ed3` and `g_header_pointer_70d4c` providers.
It follows a relative DWORD offset with the low two bits cleared, reads a WORD
count and traverses sixteen-byte records. Count zero selects one record. Native
rejection checks only `id >= signed_count`; no negative-index guard was added.
Negative position clamps to zero. Interpolation preserves low 32-bit arithmetic,
signed shifts/division and the original division fault.

Every nonzero mode enables **all seven output WORDs**, including mode `0x18`.
The final two writes read the same input lane separately; overlapping output
can change that second read. The camera previously declared `short bounds[4]`.
It now provides seven WORDs, the observed fourteen-byte minimum. The original
caller has sixteen stack bytes available there. No original aggregate declaration
or previously observed game crash is claimed.

```powershell
python scripts/eng1_bounded_differential.py camera-bounds --native-eng1 D:/Prog/hercules_recon/work/discs/pc_install/ENG1.DLL
```

Fresh VC5 testing passes 108 original/source cases and 649 checks: mode/count/key
boundaries, unsigned lanes and wrap, signed count, null-root early rejection,
valid negative-index fixture storage, duplicate/unsorted keys, four overlapping
buffers, output guards and an explicitly asserted division fault. The fixture
maps the original DLL without resolving imports or running its initializer and
calls this leaf only. No game process runs.

Regression passes 134 strict rows, 38 source fixtures, 85 Python tests and the
59-provider/923-field data audit. Raw code is **not exact**, despite equal
432-byte padded extents; masked comparison also differs. `recovery.json` is
unchanged. Finite behavior, source coverage and byte identity remain separate.

REA also inspected `0xe670` and `0x26730`. At this checkpoint their broader
state/service paths were leads; the later chain recovery supplies part of `26730`.
A same-address provider scan of the
remaining direct-call names found no already-compiled peers to bind automatically.
